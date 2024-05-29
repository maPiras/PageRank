#include "xerrori.h"
#include "prototypes.h"

double *pagerank(grafo *g,double d, double eps, int maxiter, int taux, int *numiter){
  double nodes_number = g->N;
  
  double *x = malloc(nodes_number*sizeof(double));
  for(int i=0; i<g->N; i++) x[i]=1/nodes_number;

  double *y = malloc(nodes_number*sizeof(double));
  for(int i=0; i<g->N; i++){
    if(g->out[i]>0)
    y[i]=x[i]/g->out[i];
    else
    y[i]=0;
  }
  double *xnext = malloc(nodes_number*sizeof(double));

  pthread_t t[taux];
  dati_calcolatori dati[taux];

  pthread_mutex_t imutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

  vector_cond vector_index;
  vector_index.cv = &cv;
  vector_index.imutex = &imutex;
  vector_index.index = 0;

  double term1 = (1-d)/nodes_number;

  double St = 0;
  for(int i=0; i<nodes_number; i++){
    if(g->out[i] == 0) St += x[i];
  }

  double errore = 0;
  int iter= 0;

  

  aux cond_aux_edit;
  cond_aux_edit.aux_index = 0;

  terminated cond_terminated;
  cond_terminated.terminated = 0;


  for(int i=0; i<taux; i++){
    dati[i].graph = g;
    dati[i].x = x;
    dati[i].y = y;
    dati[i].St = &St;
    dati[i].xnext = xnext;
    dati[i].term1 = term1;
    dati[i].dump = d;
    dati[i].iter = &iter;
    dati[i].errore = &errore;
    dati[i].vector_cond = &vector_index;

    xpthread_create(&t[i], NULL, &tbody_calcolo, &dati[i], QUI);
  }

  while(iter<=1 && errore < eps){
    xpthread_mutex_lock(vector_index.imutex,QUI);
    while(vector_index.index < nodes_number){
      xpthread_cond_wait(vector_index.cv,vector_index.imutex,QUI);
    }
    vector_index.index = 0;
    errore = 0;
    xpthread_cond_signal(vector_index.cv,QUI);
    xpthread_mutex_unlock(vector_index.imutex,QUI);

    iter++;
  }
  vector_index.index = -1;

  for(int i=0; i<taux; i++) xpthread_join(t[i],NULL,QUI);

  return xnext;
}

