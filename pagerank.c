#include "xerrori.h"
#include "prototypes.h"

double *pagerank(grafo *g,double d, double eps, int maxiter, int taux, int *numiter){
  
  double *x = malloc((g->N)*sizeof(double));
  for(int i=0; i<g->N; i++) x[i]=1/g->N;
  
  double *y = malloc((g->N)*sizeof(double));
  for(int i=0; i<g->N; i++) y[i]=x[i]/g->out[i];
  
  double *xnext = malloc((g->N)*sizeof(double));

  double term1 = (1-d)/g->N;
  int gindex = 0;
  int errore = 0;

  int iter=0;

  pthread_t t[taux];
  dati_calcolatori dati[taux];

  pthread_mutex_t vmutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t imutex = PTHREAD_MUTEX_INITIALIZER;
  
  for(int i=0; i<taux; i++){
    dati[i].graph = g;
    dati[i].vmutex = &vmutex;
    dati[i].x = x;
    dati[i].y = y;
    dati[i].xnext = xnext;
    dati[i].index = &gindex;
    dati[i].imutex = &imutex;
    dati[i].term = term1;
    dati[i].dump = d;

   xpthread_create(&t[i], NULL, &tbody_calcolo, &dati[i], QUI);
  }
  
  while(iter<maxiter && errore >= eps){
    
  }

}