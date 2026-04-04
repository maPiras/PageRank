/* ============================================================================
 * main.c
 * Entry point for the PageRank tool.
 *
 * Parses command-line options, builds the graph from a Matrix Market (.mtx)
 * file, runs the PageRank algorithm, and prints the top-K ranked nodes.
 *
 * Usage:
 *   pagerank [-k K] [-m M] [-d D] [-e E] [-t T] <infile>
 * ============================================================================ */

#include "../headers/errcheck.h"
#include "../headers/prototypes.h"

int main(int argc, char *argv[]) {
    int opt;

    /* Algorithm parameters with their default values. */
    int    K = TOP_NODES;      /* Number of top nodes to display             */
    int    M = MAX_ITERATIONS; /* Maximum number of iterations               */
    double D = DAMPING;        /* Damping factor                             */
    double E = MAX_ERROR;      /* Convergence threshold                      */
    int    T = THREADS;        /* Number of worker threads                   */

    /* Parse optional flags. */
    while ((opt = getopt(argc, argv, "k:m:d:e:t:")) != -1) {
        switch (opt) {
        case 'k': K = atoi(optarg); break;
        case 'm': M = atoi(optarg); break;
        case 'd': D = atof(optarg); break;
        case 'e': E = atof(optarg); break;
        case 't': T = atoi(optarg); break;
        default:
            print_usage();
            exit(1);
        }
    }

    /* Exactly one positional argument (the input file) must remain. */
    if (optind + 1 != argc) {
        print_usage();
        exit(1);
    }

    /* Build the graph from the .mtx file using T threads. */
    graph_t *g = build_graph(argv[argc - 1], T);

    /* Print basic graph statistics (node count, dead-ends, valid arcs). */
    print_graph_stats(g);

    /* Run the PageRank algorithm. */
    int     num_iter;
    double *ranks = pagerank(g, D, E, M, T, &num_iter);

    /* Verify that ranks sum to 1 (sanity check). */
    double rank_sum = 0.0;
    for (int i = 0; i < g->num_nodes; i++)
        rank_sum += ranks[i];

    if (num_iter < M)
        printf("Converged after %d iterations\n", num_iter);
    else
        printf("Did not converge after %d iterations\n", M);

    printf("Sum of ranks: %.4f   (should be 1)\n", rank_sum);

    /* Build a sortable array of (node_id, rank) pairs and sort descending. */
    rank_entry_t *sorted = malloc(sizeof(rank_entry_t) * g->num_nodes);
    for (int i = 0; i < g->num_nodes; i++) {
        sorted[i].node_id = i;
        sorted[i].rank    = ranks[i];
    }
    qsort(sorted, g->num_nodes, sizeof(rank_entry_t), compare_rank_desc);

    /* Print the top-K nodes. */
    printf("Top %d nodes:\n", K);
    for (int i = 0; i < K; i++)
        printf("  node %-6d  rank %.8f\n", sorted[i].node_id, sorted[i].rank);

    /* Clean up. */
    free_graph(g);
    free(sorted);
    free(ranks);

    return 0;
}
