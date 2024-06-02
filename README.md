# Progetto Pagerank - Laboratorio II 2024

La fase del calcolo del pagerank si divide in due step: il primo consiste nella consumazione dell'indice da parte del thread.
Per questo step è predisposta una condition variable accoppiata ad una mutex le quali consentono ai thread di accedere all'indice in maniera atomica. 
La mutex viene bloccata per il tempo strettamente necessario a salvare l'indice in una variabile locale e incrementare quello condiviso.
La mutex viene poi sbloccata per consentire agli altri thread di procerere con gli indici successivi, nel frattempo chi ha consumato l'indice ha proceduto coi calcoli.

```C
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
```
Il secondo step ha inizio immediatamente dopo il calcolo delle componenti di xnext e delle variabili ausiliarie, di queste St_new la quale, poichè condivisa, viene incrementata sotto mutex;

Questo passaggio prevede l'incremento del contatore dei thread che hanno terminato il calcolo oltre che la verifica del massimo.

La fase è necessaria affinchè il main thread sia in grado di capire quando può effettuare lo swap tra i dati strutturati ausiliari e i vettori che dovranno essere completamente aggiornati prima della nuova iterazione.

```C
xpthread_mutex_lock(dati->terminated_cond->mutex,QUI);

    dati->terminated_cond->terminated += 1;
    *(dati->errore) += fabs(dati->xnext[thread_vector_index] - dati->x[thread_vector_index]);
    if(dati->xnext[thread_vector_index] > dati->massimo->rank){
      dati->massimo->rank = dati->xnext[thread_vector_index];
      dati->massimo->indice = thread_vector_index;
    }
    xpthread_cond_signal(dati->terminated_cond->cv,QUI);
    xpthread_mutex_unlock(dati->terminated_cond->mutex,QUI);
```
