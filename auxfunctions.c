#include "prototypes.h"
#include "xerrori.h"
#include <stdio.h>

void *tbody_scrittura(void *arg) {
  dati_consumatori *d = (dati_consumatori *)arg;
  arco arch;

  do {
    xsem_wait(d->items, QUI);
    xpthread_mutex_lock(d->bmutex, QUI);
    arch.from = d->buffer[*(d->cbindex) % BUFFSIZE].from;
    arch.to = d->buffer[*(d->cbindex) % BUFFSIZE].to;
    *(d->cbindex) += 1;
    xpthread_mutex_unlock(d->bmutex, QUI);
    xsem_post(d->free, QUI);

    if (arch.from == -1)
      pthread_exit(NULL);

    xpthread_mutex_lock(d->gmutex, QUI);
    inserisci(d->graph, arch);
    xpthread_mutex_unlock(d->gmutex, QUI);
  } while (true);

  pthread_exit(NULL);
}

void *tbody_calcolo(void *arg) {
  dati_calcolatori *dati = (dati_calcolatori *)arg;
  int thread_vector_index;

  while(true){
    xpthread_mutex_lock(dati->vector_cond->imutex,QUI);
    while(dati->vector_cond->index >= dati->graph->N){
      xpthread_cond_wait(dati->vector_cond->cv,dati->vector_cond->imutex,QUI);
    }
    thread_vector_index = dati->vector_cond->index;
    if(dati->vector_cond->index < 0){
      xpthread_cond_signal(dati->vector_cond->cv,QUI);
      xpthread_mutex_unlock(dati->vector_cond->imutex,QUI);

      

      break;
    } else {
    dati->vector_cond->index += 1;
    xpthread_cond_signal(dati->vector_cond->cv,QUI);
    xpthread_mutex_unlock(dati->vector_cond->imutex,QUI);
    }

    double term2 = 0;

    for(inmap *i = dati->graph->in[thread_vector_index];i != NULL;i=i->next){
      term2 += dati->y[i->N];
    }

    term2 = term2 * dati->dump;

    dati->xnext[thread_vector_index] = dati->term1 + term2 + ((dati->dump)/(dati->graph->N)) * (*(dati->St));

    //*(dati->errore) += abs(dati->xnext[thread_vector_index] - dati->x[thread_vector_index]);
  }
  return NULL;
}

void inserisci(grafo *graph, arco arch) {
  if (arch.from != arch.to) {
    inmap *nuovo = malloc(sizeof(inmap));
    nuovo->N = arch.from;
    nuovo->next = NULL;

    if (graph->in[arch.to] == NULL) {
      graph->in[arch.to] = nuovo;
      graph->out[arch.from] += 1;
    } else {
      inmap *i;
      for (i = graph->in[arch.to]; i->next != NULL && i->N != arch.from; i = i->next);
      if (i->N != arch.from) {
        i->next = nuovo;
        graph->out[arch.from] += 1;
      }
    }
  }
}

void nodes_dead_end_valid_arcs(grafo *grafo) {
  printf("Number of nodes: %d\n", grafo->N);

  int deadend = 0;

  for (int i = 0; i < grafo->N; i++)
    if (grafo->out[i] == 0)
      deadend++;

  int valid = 0;

  printf("Number of dead-end nodes: %d\n", deadend);

  for (int i = 0; i < grafo->N; i++) {
    inmap *l = grafo->in[i];
    if (grafo->in[i] != NULL) {
      valid++;
      while (l->next != NULL) {
        valid++;
        l = l->next;
      }
    }
  }
  printf("Number of valid arcs: %d\n", valid);
}

void help() {
  fprintf(stderr, "pagerank [-k K] [-m M] [-d D] [-e E] infile\n");
  fprintf(stderr, "positional arguments:\n  infile \t input file\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "-k K show top K nodes (default 3)\n");
  fprintf(stderr, "-m M maximum number of iterations (default 100)\n");
  fprintf(stderr, "-d D damping factor (default 0.9)\n");
  fprintf(stderr, "-e E max error (default 1.0e-7)\n");
  fprintf(stderr, "-t T number of auxiliary threads (default 3)\n");
}