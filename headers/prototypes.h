/* ============================================================================
 * prototypes.h
 * Shared type definitions and function prototypes for the PageRank engine.
 * ============================================================================ */

#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include <pthread.h>
#include <semaphore.h>

/* --------------------------------------------------------------------------
 * Compile-time configuration defaults (overridable via CLI flags)
 * -------------------------------------------------------------------------- */
#define BUFF_SIZE      30       /* Producer-consumer buffer capacity           */
#define TOP_NODES      3        /* Number of top-ranked nodes to display       */
#define MAX_ITERATIONS 100      /* Maximum PageRank iterations before stopping */
#define DAMPING        0.9      /* Damping factor (d) in the PageRank formula  */
#define MAX_ERROR      1.e-07   /* Convergence threshold (L1 norm of delta)    */
#define THREADS        3        /* Default number of worker threads            */

/* --------------------------------------------------------------------------
 * Graph representation
 * -------------------------------------------------------------------------- */

/* A directed edge from node `from` to node `to` (0-based indices). */
typedef struct {
    int from;
    int to;
} edge_t;

/* Node in a singly-linked list of incoming neighbours. */
typedef struct in_node {
    int node_id;            /* Index of the incoming neighbour               */
    struct in_node *next;   /* Next node in the list                         */
} in_node_t;

/* Adjacency representation of a directed graph.
 *   out_degree[i] : number of outgoing edges from node i
 *   in_list[i]    : linked list of nodes that have an edge into node i     */
typedef struct {
    int       num_nodes;
    int      *out_degree;
    in_node_t **in_list;
} graph_t;

/* --------------------------------------------------------------------------
 * Synchronisation helpers (shared between threads)
 * -------------------------------------------------------------------------- */

/* Shared counter protected by a mutex + condition variable.
 * Used to distribute node indices among compute threads in round-robin. */
typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t  *cv;
    int              index;   /* Next node index to be processed; -1 = done */
} shared_index_t;

/* Tracks how many threads have completed the current iteration. */
typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t  *cv;
    int              count;   /* Number of threads done this iteration       */
} completion_t;

/* --------------------------------------------------------------------------
 * Thread argument structs
 * -------------------------------------------------------------------------- */

/* Arguments passed to each graph-writer (consumer) thread during graph
 * construction via the producer-consumer pattern. */
typedef struct {
    graph_t         *g;         /* Target graph being built                  */
    pthread_mutex_t *bmutex;    /* Mutex protecting the shared edge buffer   */
    pthread_mutex_t *gmutex;    /* Mutex protecting the graph data structure */
    sem_t           *items;     /* Semaphore: slots filled in the buffer     */
    sem_t           *free_slots;/* Semaphore: slots free in the buffer       */
    int             *cbindex;   /* Consumer's current buffer read position   */
    edge_t          *buffer;    /* Shared circular edge buffer               */
} consumer_data_t;

/* A (node, rank) pair used for sorting results. */
typedef struct {
    int    node_id;
    double rank;
} rank_entry_t;

/* Arguments passed to each PageRank compute thread. */
typedef struct {
    graph_t *g;

    double *x;          /* Rank vector from the previous iteration           */
    double *y;          /* y[i] = x[i] / out_degree[i] (previous iter)      */
    double *y_aux;      /* y[i] for the current iteration (being computed)   */
    double *xnext;      /* Rank vector for the current iteration             */

    double *dangling_sum;       /* Sum of ranks of dangling nodes (prev iter)*/
    double *dangling_sum_new;   /* Sum of ranks of dangling nodes (curr iter)*/

    double term1;       /* Precomputed (1 - d) / N                          */
    double damping;     /* Damping factor d                                 */

    int    *iter;       /* Pointer to the current iteration counter         */
    double *error;      /* Accumulated L1 error for convergence check       */

    shared_index_t *node_idx;   /* Shared node-index dispatcher             */
    completion_t   *completion; /* Completion barrier for each iteration    */
    pthread_mutex_t *aux_mutex; /* Mutex for updating dangling_sum_new      */
    rank_entry_t    *max_node;  /* Tracks the highest-ranked node so far    */
} compute_data_t;

/* Arguments for the signal-handler thread. */
typedef struct {
    rank_entry_t    *max_node;  /* Current top-ranked node                  */
    int             *iteration; /* Pointer to the current iteration counter */
    pthread_mutex_t *mutex;     /* Mutex shared with the main loop          */
} handler_data_t;

/* --------------------------------------------------------------------------
 * Function prototypes
 * -------------------------------------------------------------------------- */

/* Graph construction (graph_gen.c) */
graph_t *build_graph(const char *filepath, int num_threads);
void     insert_edge(graph_t *g, edge_t e);
void    *writer_thread(void *arg);

/* PageRank algorithm (pagerank.c) */
double  *pagerank(graph_t *g, double damping, double eps,
                  int maxiter, int num_threads, int *num_iter);
void    *compute_thread(void *arg);

/* Utility functions (auxfunctions.c) */
void  print_graph_stats(graph_t *g);
int   compare_rank_desc(const void *a, const void *b);
void  print_usage(void);
void  free_graph(graph_t *g);
void *signal_handler_thread(void *arg);

#endif /* PROTOTYPES_H */
