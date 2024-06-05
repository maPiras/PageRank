#include "../headers/xerrori.h"
#include "../headers/prototypes.h"

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
    }
  }

  if (optind + 1 != argc) {
    help();
    exit(1);
  }

  grafo *g = crea_grafo(argv[argc - 1], T);

  nodes_dead_end_valid_arcs(g);

  int numit;
  double *vector = pagerank(g,D,E,M,T,&numit);

  double sum = 0;
  for(int i=0; i<g->N; i++)
  sum+=vector[i];

  if(numit < M)
  printf("Converged after %d iterations\n",numit-1);
  else
  printf("Did not converge after %d iterations\n",M);

  printf("Sum of ranks: %.4f (should be 1)\n",sum);

  coppia_indice *vector_index = malloc(sizeof(coppia_indice)*g->N);
  for(int i=0; i<g->N; i++){
    vector_index[i].indice = i;
    vector_index[i].rank = vector[i];
  }

  qsort(vector_index,g->N,sizeof(coppia_indice),compare);

  printf("Top %d nodes:\n",K);
  for(int i=0; i<K; i++) printf("  %d %f\n",vector_index[i].indice,vector_index[i].rank);

  deallocate(g);
  free(vector_index);
  free(vector);

  return 0;
}