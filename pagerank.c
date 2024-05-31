#include "xerrori.h"
#include "prototypes.h"

double *pagerank(grafo *g,double d, double eps, int maxiter, int taux, int *numiter){

  sigset_t mask;
  sigfillset(&mask);  
  sigdelset(&mask,SIGUSR2);
  pthread_sigmask(SIG_BLOCK,&mask,NULL);

  pthread_t gestore;

  xpthread_create(&gestore,NULL,handler_body,NULL,QUI);

  double nodes_number = (double)g->N;
  
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
  double *y_aux = malloc(nodes_number*sizeof(double));

  pthread_t t[taux];
  dati_calcolatori dati[taux];

  pthread_mutex_t v_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t v_cv = PTHREAD_COND_INITIALIZER;

  vector_cond vector_index;

  vector_index.cv = &v_cv;
  vector_index.mutex = &v_mutex;
  vector_index.index = 0;

  double term1 = (1-d)/nodes_number;
  double St = 0;
  double St_new = 0;
  
  double errore = 0;
  int iter= 0;

  for(int i=0; i<nodes_number; i++){
    if(g->out[i] == 0) St += x[i];
  }

  pthread_mutex_t aux = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t t_cv = PTHREAD_COND_INITIALIZER;

  terminated cond_terminated;

  cond_terminated.mutex = &t_mutex;
  cond_terminated.cv = &t_cv;
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
    dati[i].y_aux = y_aux;
    dati[i].St_new = &St_new;
    dati[i].terminated_cond = &cond_terminated;
    dati[i].aux = &aux;
    dati[i].massimo = &massimo;

    xpthread_create(&t[i], NULL, &tbody_calcolo, &dati[i], QUI);
  }

  do{
    xpthread_mutex_lock(&t_mutex,QUI);
    while(cond_terminated.terminated != nodes_number){
      xpthread_cond_wait(cond_terminated.cv,cond_terminated.mutex,QUI);
    }
    cond_terminated.terminated=0;
    xpthread_mutex_unlock(&t_mutex,QUI);

    if(errore<eps){
      xpthread_mutex_lock(vector_index.mutex,QUI);
      vector_index.index = -1;
      xpthread_cond_signal(vector_index.cv,QUI);
      xpthread_mutex_unlock(vector_index.mutex,QUI);
      break;
    }
      

    St = St_new;
    for(int i=0; i<nodes_number; i++){
       y[i] = y_aux[i];
       x[i] = xnext[i];
    }
    //fprintf(stderr,"SWAPPP %d - err: %f\n",iter,errore);
    
    xpthread_mutex_lock(vector_index.mutex,QUI);
    while(vector_index.index < nodes_number){
      xpthread_cond_wait(vector_index.cv,vector_index.mutex,QUI);
    }

    St_new=0;
    errore = 0;
    vector_index.index = 0;
    xpthread_cond_signal(vector_index.cv,QUI);
    xpthread_mutex_unlock(vector_index.mutex,QUI);
    
    iter++;

  }while(iter<=maxiter);

  *numiter = iter;
  vector_index.index = -1;

  for(int i=0; i<taux; i++) xpthread_join(t[i],NULL,QUI);

  free(x);
  free(y);
  free(y_aux);

  xpthread_mutex_destroy(&Stm,QUI);
  xpthread_mutex_destroy(&t_mutex,QUI);
  xpthread_mutex_destroy(&v_mutex,QUI);
  xpthread_cond_destroy(&v_cv,QUI);
  xpthread_cond_destroy(&t_cv,QUI);

  return xnext;
}

