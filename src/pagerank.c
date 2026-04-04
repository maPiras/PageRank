/* ============================================================================
 * pagerank.c
 * Iterative PageRank computation using a multi-threaded power-iteration
 * method.
 *
 * Algorithm overview
 * ------------------
 * The standard PageRank formula with dangling-node handling:
 *
 *   x_new[i] = (1-d)/N  +  d * sum_{j->i} ( x[j] / out(j) )
 *                        +  d/N * St
 *
 * where:
 *   d    = damping factor
 *   N    = total number of nodes
 *   St   = sum of ranks of dangling nodes (out-degree == 0) from the
 *          previous iteration
 *
 * The iteration stops when either the L1 norm ||x_new - x||_1 < eps or the
 * maximum number of iterations is reached.
 *
 * Threading model
 * ---------------
 * `num_threads` compute threads are spawned once and reused for every
 * iteration.  A shared_index_t dispatcher hands each thread a unique node
 * index to process; when all N nodes are processed the main loop resets the
 * dispatcher for the next iteration.  A dedicated signal-handler thread
 * handles SIGUSR1 (print progress) and SIGTERM (clean shutdown).
 * ============================================================================ */

#include "../headers/errcheck.h"
#include "../headers/prototypes.h"

/* --------------------------------------------------------------------------
 * pagerank
 *
 * Parameters:
 *   g           — input graph
 *   damping     — damping factor d  (typically 0.85–0.9)
 *   eps         — convergence threshold (L1 norm)
 *   maxiter     — maximum number of iterations
 *   num_threads — number of parallel compute threads
 *   num_iter    — [out] actual number of iterations performed
 *
 * Returns a heap-allocated array of N rank values (caller must free).
 * -------------------------------------------------------------------------- */
double *pagerank(graph_t *g, double damping, double eps,
                 int maxiter, int num_threads, int *num_iter) {
    fprintf(stderr, "Starting PageRank computation...\n");

    int    n   = g->num_nodes;
    double err = eps;
    int    iter = 0;

    /* --- Synchronisation primitives ---
     * t_mutex / t_cv   : shared with the signal-handler thread
     * v_mutex / v_cv   : guard the shared node-index dispatcher              */
    pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  t_cv    = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t v_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  v_cv    = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t aux_mutex = PTHREAD_MUTEX_INITIALIZER;

    /* Block SIGUSR1 and SIGTERM in the main (and subsequently worker) threads
     * so that only the dedicated signal-handler thread will receive them.     */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    /* --- Rank vectors ---
     * x      : ranks from the previous iteration
     * xnext  : ranks being computed for the current iteration (returned)
     * y      : y[i] = x[i] / out_degree[i]  (previous iter, for fast lookup)
     * y_aux  : y[i] for the current iteration (written by threads, swapped in)*/
    double *x     = malloc(n * sizeof(double));
    double *xnext = malloc(n * sizeof(double));
    double *y     = malloc(n * sizeof(double));
    double *y_aux = malloc(n * sizeof(double));

    /* --- Dangling-node sums ---
     * dangling_sum      : St from the previous iteration (read by threads)
     * dangling_sum_new  : St being accumulated for the current iteration     */
    double dangling_sum     = 0.0;
    double dangling_sum_new = 0.0;

    /* Precomputed constant: (1 - d) / N */
    double term1 = (1.0 - damping) / (double)n;

    /* --- Current top-ranked node (updated by compute threads each iter) --- */
    rank_entry_t cur_max  = { .node_id = 0,  .rank = 0.0  };
    rank_entry_t next_max = { .node_id = -1, .rank = -1.0 };

    /* --- Signal-handler thread --- */
    pthread_t      handler_tid;
    handler_data_t handler_args = {
        .max_node  = &cur_max,
        .iteration = &iter,
        .mutex     = &t_mutex,
    };
    xpthread_create(&handler_tid, NULL, signal_handler_thread, &handler_args, QUI);

    /* --- Shared node-index dispatcher --- */
    shared_index_t node_idx = {
        .mutex = &v_mutex,
        .cv    = &v_cv,
        .index = 0,
    };

    /* --- Completion barrier (counts threads done per iteration) --- */
    completion_t completion = {
        .mutex = &t_mutex,
        .cv    = &t_cv,
        .count = 0,
    };

    /* --- Spawn compute threads --- */
    pthread_t     threads[num_threads];
    compute_data_t thread_args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].g                = g;
        thread_args[i].x                = x;
        thread_args[i].y                = y;
        thread_args[i].y_aux            = y_aux;
        thread_args[i].xnext            = xnext;
        thread_args[i].dangling_sum     = &dangling_sum;
        thread_args[i].dangling_sum_new = &dangling_sum_new;
        thread_args[i].term1            = term1;
        thread_args[i].damping          = damping;
        thread_args[i].iter             = &iter;
        thread_args[i].error            = &err;
        thread_args[i].node_idx         = &node_idx;
        thread_args[i].completion       = &completion;
        thread_args[i].aux_mutex        = &aux_mutex;
        thread_args[i].max_node         = &next_max;
        xpthread_create(&threads[i], NULL, &compute_thread, &thread_args[i], QUI);
    }

    /* -----------------------------------------------------------------------
     * Main iteration loop
     * ----------------------------------------------------------------------- */
    do {
        /* Wait until all N nodes have been processed this iteration. */
        xpthread_mutex_lock(&t_mutex, QUI);
        while (completion.count != n)
            xpthread_cond_wait(completion.cv, completion.mutex, QUI);

        /* Reset the completion counter for the next iteration. */
        completion.count = 0;

        /* Update the top-ranked node for the signal handler. */
        cur_max.node_id   = next_max.node_id;
        cur_max.rank      = next_max.rank;
        next_max.rank     = -1.0;
        xpthread_mutex_unlock(&t_mutex, QUI);

        /* Check convergence. */
        if (err < eps) {
            /* Signal all threads to exit by setting index = -1. */
            xpthread_mutex_lock(node_idx.mutex, QUI);
            node_idx.index = -1;
            xpthread_cond_signal(node_idx.cv, QUI);
            xpthread_mutex_unlock(node_idx.mutex, QUI);
            break;
        }

        /* Wait until all threads have finished reading x/y before swapping. */
        if (iter > 0) {
            dangling_sum = dangling_sum_new;
            for (int i = 0; i < n; i++) {
                y[i] = y_aux[i];
                x[i] = xnext[i];
            }
        }

        /* Wait until the index dispatcher has been fully consumed this round. */
        xpthread_mutex_lock(node_idx.mutex, QUI);
        while (node_idx.index < n)
            xpthread_cond_wait(node_idx.cv, node_idx.mutex, QUI);

        /* Reset for the next iteration and wake one waiting thread. */
        dangling_sum_new = 0.0;
        err              = 0.0;
        node_idx.index   = 0;

        xpthread_cond_signal(node_idx.cv, QUI);
        xpthread_mutex_unlock(node_idx.mutex, QUI);

        iter++;

    } while (iter <= maxiter);

    *num_iter = iter;

    /* Ensure threads see index == -1 in case we exited via maxiter. */
    node_idx.index = -1;

    /* Join all compute threads. */
    for (int i = 0; i < num_threads; i++)
        xpthread_join(threads[i], NULL, QUI);

    /* Shut down the signal-handler thread. */
    pthread_kill(handler_tid, SIGTERM);
    xpthread_join(handler_tid, NULL, QUI);

    /* Free intermediate buffers; xnext is returned to the caller. */
    free(x);
    free(y);
    free(y_aux);

    xpthread_mutex_destroy(&aux_mutex, QUI);
    xpthread_mutex_destroy(&t_mutex,   QUI);
    xpthread_mutex_destroy(&v_mutex,   QUI);
    xpthread_cond_destroy(&v_cv,       QUI);
    xpthread_cond_destroy(&t_cv,       QUI);

    return xnext;
}
