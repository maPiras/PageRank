#include "../headers/xerrori.h"
#include "../headers/prototypes.h"

double *pagerank(grafo *g,double d, double eps, int maxiter, int taux, int *numiter){
  fprintf(stderr,"Inizio calcolo pagerank...\n");

  pthread_mutex_t aux = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t t_cv = PTHREAD_COND_INITIALIZER;

  sigset_t mask;
  sigemptyset(&mask);  
  sigaddset(&mask,SIGUSR1);
  sigaddset(&mask,SIGTERM);
  pthread_sigmask(SIG_BLOCK,&mask,NULL);

  pthread_t gestore;

  double nodes_number = (double)g->N;
  
  double *x = malloc(nodes_number*sizeof(double));
  double *y = malloc(nodes_number*sizeof(double));

  double *xnext = malloc(nodes_number*sizeof(double));
  double *y_aux = malloc(nodes_number*sizeof(double));

  double errore = eps;
  int iter = 0;

  coppia_indice massimo;
  massimo.indice = 0;
  massimo.rank = x[0];

  coppia_indice massimo_next;
  massimo_next.indice = -1;
  massimo_next.rank = -1;

  handler_data dati_gestore;
  dati_gestore.massimo = &massimo;
  dati_gestore.iterazione = &iter;
  dati_gestore.mutex = &t_mutex;

  xpthread_create(&gestore,NULL,handler_body,&dati_gestore,QUI);

  pthread_t t[taux];
  dati_calcolatori dati[taux];

  pthread_mutex_t v_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t v_cv = PTHREAD_COND_INITIALIZER;

  vector_cond vector_index;
  vector_index.cv = &v_cv;
  vector_index.mutex = &v_mutex;
  vector_index.index = 0;

  terminated cond_terminated;
  cond_terminated.mutex = &t_mutex;
  cond_terminated.cv = &t_cv;
  cond_terminated.terminated = 0;

  double term1 = (1-d)/nodes_number;
  double St = 0;
  double St_new = 0;

  for(int i=0; i<nodes_number; i++){
    if(g->out[i] == 0) St += x[i];
  }

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
    dati[i].y_aux = y_aux;
    dati[i].St_new = &St_new;
    dati[i].terminated_cond = &cond_terminated;
    dati[i].aux = &aux;
    dati[i].massimo = &massimo_next;

    xpthread_create(&t[i], NULL, &tbody_calcolo, &dati[i], QUI);
  }

  do{
    xpthread_mutex_lock(&t_mutex,QUI);
    while(cond_terminated.terminated != nodes_number){
      xpthread_cond_wait(cond_terminated.cv,cond_terminated.mutex,QUI);
    }

    if(iter > 0){
      St = St_new;
      for(int i=0; i<nodes_number; i++){
        y[i] = y_aux[i];
        x[i] = xnext[i];
      }
    }

    cond_terminated.terminated=0;
    massimo.indice = massimo_next.indice;
    massimo.rank = massimo_next.rank;
    massimo_next.rank = -1;
    iter++;
    
    xpthread_mutex_unlock(&t_mutex,QUI);

    if(errore<eps){
      xpthread_mutex_lock(vector_index.mutex,QUI);
      vector_index.index = -1;
      xpthread_cond_signal(vector_index.cv,QUI);
      xpthread_mutex_unlock(vector_index.mutex,QUI);
      break;
    }
    
    xpthread_mutex_lock(vector_index.mutex,QUI);
    while(vector_index.index < nodes_number){
      xpthread_cond_wait(vector_index.cv,vector_index.mutex,QUI);
    }

    St_new = 0; //Azzero l'accumulatore del fattore St per la nuova iterazione
    errore = 0; //Azzero l'errore per la nuova iterazione
    vector_index.index = 0; //Ricomincio dal nodo zero '' ''

    xpthread_cond_signal(vector_index.cv,QUI);
    xpthread_mutex_unlock(vector_index.mutex,QUI);

  }while(iter<maxiter);

  *numiter = iter;
  vector_index.index = -1;

   for(int i=0; i<taux; i++) xpthread_join(t[i],NULL,QUI);

  pthread_kill(gestore,SIGTERM);
  xpthread_join(gestore,NULL,QUI);

  free(x);
  free(y);
  free(y_aux);

  xpthread_mutex_destroy(&aux,QUI);
  xpthread_mutex_destroy(&t_mutex,QUI);
  xpthread_mutex_destroy(&v_mutex,QUI);
  xpthread_cond_destroy(&v_cv,QUI);
  xpthread_cond_destroy(&t_cv,QUI);

  return xnext;
}

