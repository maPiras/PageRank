#include "../headers/xerrori.h"
#include "../headers/prototypes.h"


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
      return(NULL);

    xpthread_mutex_lock(d->gmutex, QUI);

    inserisci(d->graph, arch);
    
    xpthread_mutex_unlock(d->gmutex, QUI);

  } while (true);

  return(NULL);
}

void *tbody_calcolo(void *arg) {
  dati_calcolatori *dati = (dati_calcolatori *)arg;
  int thread_vector_index = 0;

  while(true){
    
    xpthread_mutex_lock(dati->vector_cond->mutex,QUI);
      while(dati->vector_cond->index >= dati->graph->N){
        xpthread_cond_wait(dati->vector_cond->cv,dati->vector_cond->mutex,QUI);
      }
      thread_vector_index = dati->vector_cond->index;
      if(dati->vector_cond->index < 0){
        xpthread_cond_signal(dati->vector_cond->cv,QUI);
        xpthread_mutex_unlock(dati->vector_cond->mutex,QUI);
        break;
      } else {
      dati->vector_cond->index += 1;

    xpthread_mutex_unlock(dati->vector_cond->mutex,QUI);
    }

    if(*(dati->iter) != 0){
      double term2 = 0.0;
      double term3 = 0.0;
      
      for(inmap *i = dati->graph->in[thread_vector_index];i != NULL;i=i->next){
        term2 += dati->y[i->N];
      }

      term2 = term2 * dati->dump;

      term3 = ((dati->dump)/(double)(dati->graph->N)) * (*(dati->St));

      dati->xnext[thread_vector_index] = dati->term1 + term2 + term3;
      if(dati->graph->out[thread_vector_index] != 0)
      dati->y_aux[thread_vector_index] = dati->xnext[thread_vector_index]/(double)dati->graph->out[thread_vector_index];
      else{
        xpthread_mutex_lock(dati->aux,QUI);
        *(dati->St_new) += dati->xnext[thread_vector_index];
        xpthread_mutex_unlock(dati->aux,QUI);
      }
    } else{
      dati->x[thread_vector_index]=(double)1/dati->graph->N;
      dati->xnext[thread_vector_index] = 0;

      if(dati->graph->out[thread_vector_index]>0)
        dati->y[thread_vector_index]=dati->x[thread_vector_index]/(double)dati->graph->out[thread_vector_index];
      else dati->y[thread_vector_index]=0;

      if(dati->graph->out[thread_vector_index] == 0) 
      *(dati->St) += dati->x[thread_vector_index];
    }

    xpthread_mutex_lock(dati->terminated_cond->mutex,QUI);
    dati->terminated_cond->terminated += 1;
    *(dati->errore) += fabs(dati->xnext[thread_vector_index] - dati->x[thread_vector_index]);
    if(dati->xnext[thread_vector_index] > dati->massimo->rank){
      dati->massimo->rank = dati->xnext[thread_vector_index];
      dati->massimo->indice = thread_vector_index;
    }
    xpthread_cond_signal(dati->terminated_cond->cv,QUI);
    xpthread_mutex_unlock(dati->terminated_cond->mutex,QUI);
  }
  
  return NULL;
}

void inserisci(grafo *graph, arco arch) {
  if (arch.from != arch.to) {
    if (graph->in[arch.to] == NULL) {
      inmap *nuovo = malloc(sizeof(inmap));
      nuovo->N = arch.from;
      nuovo->next = NULL;
      graph->in[arch.to] = nuovo;
      graph->out[arch.from] += 1;
    } else {
      inmap *i;
      for (i = graph->in[arch.to]; i->next != NULL && i->N != arch.from; i = i->next);
      if (i->N != arch.from) {
        inmap *nuovo = malloc(sizeof(inmap));
        nuovo->N = arch.from;
        nuovo->next = NULL;
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

int compare(const void* p, const void* q)
{
  double l = ((struct coppia_indice*)p)->rank;
  double r = ((struct coppia_indice*)q)->rank;
  if(l>r) return -1;
  else if(l<r) return 1;
  else return 0;
}

void deallocate(grafo* g){

  for(int i=0; i<g->N; i++){
    inmap* j=g->in[i];
    inmap* l;

    while(j!=NULL){
      l=j;
      j=j->next;
      free(l);
    }
    free(j);
  }
  
  free(g->in);
  free(g->out);
  free(g);
}

void *handler_body(void *d){
  handler_data* dati = (handler_data *) d;

  int s;
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);
  sigaddset(&mask,SIGTERM);
  while(true){
    int e = sigwait(&mask,&s);
    if(e!=0) perror("Errore sigwait");
    if(s == SIGUSR1){
      xpthread_mutex_lock(dati->mutex,QUI);
      fprintf(stderr,"Iterazione corrente %d: nodo massimo %d con rank %f\n",*(dati->iterazione),dati->massimo->indice,dati->massimo->rank);
      xpthread_mutex_unlock(dati->mutex,QUI);
    } else if(s == SIGTERM) return NULL;
  }
}