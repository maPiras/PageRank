/* ============================================================================
 * graph_gen.c
 * Reads a directed graph from a Matrix Market (.mtx) file and builds an
 * in-memory adjacency structure.
 *
 * A producer-consumer pattern is used: the main thread reads edges from the
 * file (producer) and deposits them into a shared circular buffer; a pool of
 * `num_threads` writer threads (consumers) pick up edges and insert them into
 * the graph concurrently.
 * ============================================================================ */

#include "../headers/errcheck.h"
#include "../headers/prototypes.h"

/* --------------------------------------------------------------------------
 * build_graph
 *
 * Opens `filepath`, parses the Matrix Market header to learn the number of
 * nodes and edges, then spawns `num_threads` consumer threads to insert edges
 * into the graph structure.
 *
 * The .mtx format uses 1-based indices; they are converted to 0-based here.
 * Self-loops are silently discarded by insert_edge().
 * -------------------------------------------------------------------------- */
graph_t *build_graph(const char *filepath, int num_threads) {
    FILE *fp = xfopen(filepath, "r", QUI);

    char  *line = NULL;
    size_t len  = 0;

    /* Skip comment lines (lines starting with '%'). */
    while (getline(&line, &len, fp) != -1) {
        if (line[0] == '%') continue;
        break;
    }

    /* Parse the header: rows cols num_edges. */
    int rows = 0, cols = 0, num_edges = 0;
    sscanf(line, "%d %d %d", &rows, &cols, &num_edges);

    if (rows != cols)
        xtermina("Non-square matrix: rows != cols", QUI);

    /* Allocate and zero-initialise the graph. */
    graph_t *g     = malloc(sizeof(graph_t));
    g->num_nodes   = rows;
    g->out_degree  = calloc(rows, sizeof(int));
    g->in_list     = calloc(rows, sizeof(in_node_t *));

    /* Sentinel edge used to signal consumer threads to exit. */
    edge_t sentinel = { .from = -1, .to = -1 };

    /* Shared circular buffer and its synchronisation primitives. */
    edge_t buffer[BUFF_SIZE];

    pthread_mutex_t buf_mutex   = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;

    sem_t items;      /* Number of edges currently in the buffer            */
    sem_t free_slots; /* Number of free slots in the buffer                 */
    xsem_init(&items,      0, 0,         QUI);
    xsem_init(&free_slots, 0, BUFF_SIZE, QUI);

    int prod_idx = 0; /* Producer's write position                          */
    int cons_idx = 0; /* Consumer's read position (shared, protected)       */

    /* Spawn consumer threads. */
    pthread_t      threads[num_threads];
    consumer_data_t thread_args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].g          = g;
        thread_args[i].bmutex     = &buf_mutex;
        thread_args[i].gmutex     = &graph_mutex;
        thread_args[i].items      = &items;
        thread_args[i].free_slots = &free_slots;
        thread_args[i].buffer     = buffer;
        thread_args[i].cbindex    = &cons_idx;
        xpthread_create(&threads[i], NULL, &writer_thread, &thread_args[i], QUI);
    }

    /* Produce edges: read one line at a time, validate, and enqueue. */
    while (getline(&line, &len, fp) != -1) {
        edge_t e = { .from = 0, .to = 0 };
        sscanf(line, "%d %d", &e.from, &e.to);

        /* Validate bounds (MTX indices are 1-based). */
        if (e.from <= 0 || e.from > g->num_nodes ||
            e.to   <= 0 || e.to   > g->num_nodes) {

            /* Broadcast sentinels to shut down all threads, then clean up. */
            for (int i = 0; i < num_threads; i++) {
                xsem_wait(&free_slots, QUI);
                xpthread_mutex_lock(&buf_mutex, QUI);
                buffer[prod_idx % BUFF_SIZE] = sentinel;
                prod_idx++;
                xpthread_mutex_unlock(&buf_mutex, QUI);
                xsem_post(&items, QUI);
            }
            for (int i = 0; i < num_threads; i++)
                xpthread_join(threads[i], NULL, QUI);

            xpthread_mutex_destroy(&buf_mutex,   QUI);
            xpthread_mutex_destroy(&graph_mutex, QUI);
            xsem_destroy(&items,      QUI);
            xsem_destroy(&free_slots, QUI);
            fclose(fp);
            free(line);
            free_graph(g);
            xtermina("Invalid edge found in input file", QUI);
        }

        /* Convert from 1-based to 0-based indexing. */
        e.from--;
        e.to--;

        /* Enqueue the edge into the circular buffer. */
        xsem_wait(&free_slots, QUI);
        xpthread_mutex_lock(&buf_mutex, QUI);
        buffer[prod_idx % BUFF_SIZE] = e;
        prod_idx++;
        xpthread_mutex_unlock(&buf_mutex, QUI);
        xsem_post(&items, QUI);
    }

    /* Send one sentinel per consumer thread to signal end-of-input. */
    for (int i = 0; i < num_threads; i++) {
        xsem_wait(&free_slots, QUI);
        xpthread_mutex_lock(&buf_mutex, QUI);
        buffer[prod_idx % BUFF_SIZE] = sentinel;
        prod_idx++;
        xpthread_mutex_unlock(&buf_mutex, QUI);
        xsem_post(&items, QUI);
    }

    /* Wait for all consumer threads to finish. */
    for (int i = 0; i < num_threads; i++)
        xpthread_join(threads[i], NULL, QUI);

    /* Clean up synchronisation primitives and file resources. */
    xpthread_mutex_destroy(&buf_mutex,   QUI);
    xpthread_mutex_destroy(&graph_mutex, QUI);
    xsem_destroy(&items,      QUI);
    xsem_destroy(&free_slots, QUI);
    fclose(fp);
    free(line);

    return g;
}
