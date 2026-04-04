/* ============================================================================
 * auxfunctions.c
 * Thread bodies and auxiliary utility functions used by both the graph
 * construction pipeline and the PageRank algorithm.
 * ============================================================================ */

#include "../headers/errcheck.h"
#include "../headers/prototypes.h"

/* --------------------------------------------------------------------------
 * writer_thread
 *
 * Consumer thread body for the graph-construction producer-consumer pipeline.
 * Each iteration:
 *   1. Waits for an edge to become available in the shared buffer.
 *   2. Reads the edge under the buffer mutex, then releases the buffer.
 *   3. Exits when it dequeues a sentinel edge (from == -1).
 *   4. Otherwise inserts the edge into the graph under the graph mutex.
 * -------------------------------------------------------------------------- */
void *writer_thread(void *arg) {
    consumer_data_t *d = (consumer_data_t *)arg;
    edge_t e;

    do {
        /* Wait for a slot to be filled, then take it. */
        xsem_wait(d->items, QUI);
        xpthread_mutex_lock(d->bmutex, QUI);

        e.from = d->buffer[*(d->cbindex) % BUFF_SIZE].from;
        e.to   = d->buffer[*(d->cbindex) % BUFF_SIZE].to;
        *(d->cbindex) += 1;

        xpthread_mutex_unlock(d->bmutex, QUI);
        xsem_post(d->free_slots, QUI);

        /* Sentinel: producer signals end-of-input. */
        if (e.from == -1)
            return NULL;

        /* Insert the edge into the graph. */
        xpthread_mutex_lock(d->gmutex, QUI);
        insert_edge(d->g, e);
        xpthread_mutex_unlock(d->gmutex, QUI);

    } while (true);

    return NULL;
}

/* --------------------------------------------------------------------------
 * compute_thread
 *
 * Worker thread body for the PageRank iterative computation.
 *
 * Each thread repeatedly claims a node index from the shared dispatcher
 * (shared_index_t), computes that node's new rank, and signals completion.
 * The loop ends when the dispatcher emits index == -1.
 *
 * Iteration 0 is special: it initialises x[i] = 1/N and y[i] accordingly
 * instead of computing a new rank value.
 * -------------------------------------------------------------------------- */
void *compute_thread(void *arg) {
    compute_data_t *d = (compute_data_t *)arg;
    int node;

    while (true) {
        /* Claim the next node index atomically. */
        xpthread_mutex_lock(d->node_idx->mutex, QUI);
        while (d->node_idx->index >= d->g->num_nodes)
            xpthread_cond_wait(d->node_idx->cv, d->node_idx->mutex, QUI);

        node = d->node_idx->index;

        /* index == -1 signals that all iterations are done. */
        if (node < 0) {
            xpthread_cond_signal(d->node_idx->cv, QUI);
            xpthread_mutex_unlock(d->node_idx->mutex, QUI);
            break;
        }

        d->node_idx->index += 1;
        xpthread_mutex_unlock(d->node_idx->mutex, QUI);

        if (*(d->iter) > 0) {
            /* --- Normal iteration: compute xnext[node] --- */

            /* Accumulate contributions from incoming neighbours:
             *   term2 = d * sum_j ( y[j] )  for all j -> node           */
            double term2 = 0.0;
            for (in_node_t *in = d->g->in_list[node]; in != NULL; in = in->next)
                term2 += d->y[in->node_id];
            term2 *= d->damping;

            /* PageRank formula:
             *   xnext[i] = (1-d)/N  +  d * sum_j(y[j])  +  d/N * St    */
            d->xnext[node] = d->term1 + term2
                             + (d->damping / (double)d->g->num_nodes)
                               * (*(d->dangling_sum));

            if (d->g->out_degree[node] != 0) {
                /* Non-dangling node: distribute rank to successors. */
                d->y_aux[node] = d->xnext[node] / d->g->out_degree[node];
            } else {
                /* Dangling node: accumulate its rank in the new dangling sum. */
                xpthread_mutex_lock(d->aux_mutex, QUI);
                *(d->dangling_sum_new) += d->xnext[node];
                xpthread_mutex_unlock(d->aux_mutex, QUI);
            }

        } else {
            /* --- Iteration 0: uniform initialisation x[i] = 1/N --- */
            d->xnext[node]  = 0.0;
            d->x[node]      = 1.0 / (double)d->g->num_nodes;

            if (d->g->out_degree[node] > 0)
                d->y[node] = d->x[node] / d->g->out_degree[node];
            else {
                d->y[node] = 0.0;
                *(d->dangling_sum) += d->x[node];
            }
        }

        /* Signal that this thread has finished processing its node. */
        xpthread_mutex_lock(d->completion->mutex, QUI);
        d->completion->count += 1;

        if (*(d->iter) > 0) {
            /* Accumulate L1 error and track the highest-ranked node. */
            *(d->error) += fabs(d->xnext[node] - d->x[node]);

            if (d->xnext[node] > d->max_node->rank) {
                d->max_node->rank    = d->xnext[node];
                d->max_node->node_id = node;
            }
        }

        xpthread_cond_signal(d->completion->cv, QUI);
        xpthread_mutex_unlock(d->completion->mutex, QUI);
    }

    return NULL;
}

/* --------------------------------------------------------------------------
 * insert_edge
 *
 * Adds a directed edge e = (from -> to) to the graph.
 * Self-loops (from == to) are silently discarded.
 * Duplicate edges are also silently discarded.
 * -------------------------------------------------------------------------- */
void insert_edge(graph_t *g, edge_t e) {
    if (e.from == e.to) return; /* Ignore self-loops. */

    if (g->in_list[e.to] == NULL) {
        /* First incoming edge for this destination node. */
        in_node_t *node  = malloc(sizeof(in_node_t));
        node->node_id    = e.from;
        node->next       = NULL;
        g->in_list[e.to] = node;
        g->out_degree[e.from]++;
    } else {
        /* Walk the existing list; skip if this edge already exists. */
        in_node_t *curr;
        for (curr = g->in_list[e.to];
             curr->next != NULL && curr->node_id != e.from;
             curr = curr->next);

        if (curr->node_id != e.from) {
            in_node_t *node = malloc(sizeof(in_node_t));
            node->node_id   = e.from;
            node->next      = NULL;
            curr->next      = node;
            g->out_degree[e.from]++;
        }
    }
}

/* --------------------------------------------------------------------------
 * print_graph_stats
 *
 * Prints a summary of the graph: total node count, number of dangling
 * (dead-end) nodes with no outgoing edges, and total valid arc count.
 * -------------------------------------------------------------------------- */
void print_graph_stats(graph_t *g) {
    printf("Number of nodes: %d\n", g->num_nodes);

    int dead_end_count = 0;
    for (int i = 0; i < g->num_nodes; i++)
        if (g->out_degree[i] == 0)
            dead_end_count++;

    printf("Number of dead-end nodes: %d\n", dead_end_count);

    int arc_count = 0;
    for (int i = 0; i < g->num_nodes; i++) {
        for (in_node_t *curr = g->in_list[i]; curr != NULL; curr = curr->next)
            arc_count++;
    }

    printf("Number of valid arcs: %d\n", arc_count);
}

/* --------------------------------------------------------------------------
 * print_usage
 * Prints a short usage/help message to stderr.
 * -------------------------------------------------------------------------- */
void print_usage(void) {
    fprintf(stderr, "Usage: pagerank [-k K] [-m M] [-d D] [-e E] [-t T] <infile>\n");
    fprintf(stderr, "Positional arguments:\n");
    fprintf(stderr, "  infile        Input graph file in Matrix Market format\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -k K          Show top K nodes (default %d)\n",    TOP_NODES);
    fprintf(stderr, "  -m M          Maximum iterations (default %d)\n",  MAX_ITERATIONS);
    fprintf(stderr, "  -d D          Damping factor (default %.1f)\n",    DAMPING);
    fprintf(stderr, "  -e E          Convergence threshold (default %.0e)\n", MAX_ERROR);
    fprintf(stderr, "  -t T          Number of worker threads (default %d)\n", THREADS);
}

/* --------------------------------------------------------------------------
 * compare_rank_desc
 * qsort comparator: sorts rank_entry_t elements in descending order of rank.
 * -------------------------------------------------------------------------- */
int compare_rank_desc(const void *a, const void *b) {
    double ra = ((const rank_entry_t *)a)->rank;
    double rb = ((const rank_entry_t *)b)->rank;
    if (ra > rb) return -1;
    if (ra < rb) return  1;
    return 0;
}

/* --------------------------------------------------------------------------
 * free_graph
 * Recursively frees all heap memory associated with a graph_t.
 * -------------------------------------------------------------------------- */
void free_graph(graph_t *g) {
    for (int i = 0; i < g->num_nodes; i++) {
        in_node_t *curr = g->in_list[i];
        while (curr != NULL) {
            in_node_t *tmp = curr;
            curr = curr->next;
            free(tmp);
        }
    }
    free(g->in_list);
    free(g->out_degree);
    free(g);
}

/* --------------------------------------------------------------------------
 * signal_handler_thread
 *
 * Dedicated thread that waits for SIGUSR1 or SIGTERM via sigwait().
 *   SIGUSR1  — prints the current iteration and the highest-ranked node.
 *   SIGTERM  — exits the thread cleanly, allowing the main loop to join it.
 *
 * Signals must be blocked in all other threads before this thread is created.
 * -------------------------------------------------------------------------- */
void *signal_handler_thread(void *arg) {
    handler_data_t *d = (handler_data_t *)arg;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);

    while (true) {
        int sig;
        int e = sigwait(&mask, &sig);
        if (e != 0) perror("sigwait failed");

        if (sig == SIGUSR1) {
            xpthread_mutex_lock(d->mutex, QUI);
            fprintf(stderr,
                    "Iteration %d: top node %d  rank %.8f\n",
                    *(d->iteration),
                    d->max_node->node_id,
                    d->max_node->rank);
            xpthread_mutex_unlock(d->mutex, QUI);
        } else if (sig == SIGTERM) {
            return NULL;
        }
    }
}
