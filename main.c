#include "prototypes.h"
#include "xerrori.h"
#include <stdio.h>

#define TOP_NODES 3
#define MAX_ITERATIONS 100
#define DUMPING 0.9
#define MAX_ERROR 1.e-07
#define THREADS 3

int main(int argc, char *argv[]) {
  int opt;

  int K = TOP_NODES;
  int M = MAX_ITERATIONS;
  double D = DUMPING;
  double E = MAX_ERROR;
  int T = THREADS;

  while ((opt = getopt(argc, argv, "k:m:d:e:t:")) != -1) {
    switch (opt) {
    case 'k':
      K = atoi(optarg);
      break;
    case 'm':
      M = atoi(optarg);
      break;
    case 'd':
      D = atof(optarg);
      break;
    case 'e':
      E = atof(optarg);
      break;
    case 't':
      T = atoi(optarg);
      break;
    default:
      help();
      exit(1);
      break;
    }
  }

  if (optind + 1 != argc) {
    help();
    exit(1);
  }

  grafo *graph = crea_grafo(argv[argc - 1], T);

  nodes_dead_end_valid_arcs(graph);

  // Creare aux per deallocare grafo

  return 0;
}