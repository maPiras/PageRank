# Progetto Pagerank - Laboratorio II 2024

## Cartella src

**auxfunctions.c** : funzioni thread body, handler body, deallocazione, conteggio dead ends, compare, help e inserimento di un nodo nel grafo.

**prototypes.h** : prototipi delle suddette funzioni e dichiarazioni di tipi


## Calcolo

La fase del calcolo del pagerank si divide in due step: il primo consiste nella consumazione dell'indice da parte del thread.
Per questo step è predisposta una condition variable accoppiata ad una mutex le quali consentono ai thread di accedere all'indice in maniera atomica. 

La mutex viene bloccata per il tempo strettamente necessario a salvare l'indice in una variabile locale e incrementare quello condiviso.

Viene poi sbloccata per consentire agli altri thread di procedere con gli indici successivi.

In questa fase si separa il caso particolare in cui sia in corso l'iterazione 0, ovvero la fase di inizializzazione dei vettori ausiliari x e y.

Questo viene fatto verificando l'iterazione ed andando a eseguire le suddette operazioni solo nel caso in cui, appunto, questa sia la prima in assoluto.


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

Questo passaggio prevede l'incremento del contatore dei thread che hanno terminato il calcolo oltre che la verifica del nodo di rank massimo.

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

## Server python

Per quanto riguarda la gestione dei thread nel server python si è fatto uso di un threadpool:
ad ogni thread viene assegnata una funzione la quale riceve gli archi dal client uno alla volta, i quali vengono inseriti in una lista e scritti nel file temporaneo non appena questa raggiunge una lunghezza di 10 elementi.

Alla fine dell'iterazione viene effettuata in ogni caso una push nel caso in cui l'ultimo blocco sia composto da meno di 10 elementi.

```Py
    buffer.append(f"{From} {To}\n")
    
    if len(buffer) >= 10:
        for v in buffer:
            temp.write(v)
            
        buffer.clear()

if(len(buffer) != 0):
    temp.writelines(buffer)
    buffer.clear()
temp.seek(0)
```
