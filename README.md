# PageRank - "Lab II" final project

## Src folder

**auxfunctions.c** : functions thread body, handler body, deallocation, counting dead ends, compare, help and inserting a node into the graph.

**prototypes.h** : Prototypes of the above functions and type declarations


## Calculation

The pagerank calculation phase is divided into two steps: the first consists in the consumption of the index by the thread.
For this step, a condition variable coupled with a mutex is prepared which allows the threads to access the index atomically. 

The mutex is blocked for the time strictly necessary to save the index in a local variable and increase the shared one.

It is then unlocked to allow other threads to proceed with subsequent indexes.

In this phase, the particular case in which iteration 0 is in progress is separated, i.e. the initialization phase of the auxiliary vectors x and y.

This is done by checking the iteration and carrying out the aforementioned operations only if this is the first ever.


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
The second step begins immediately after the calculation of the components of xnext and the auxiliary variables, of which St_new which, since it is shared, is incremented under mutex;

This step involves increasing the counter of threads that have finished the calculation as well as verifying the maximum rank node.

The phase is necessary so that the main thread is able to understand when it can swap between the auxiliary structured data and the vectors that will have to be completely updated before the new iteration.

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

## Python Server

As regards the management of threads in the Python server, a threadpool was used:
each thread is assigned a function which receives the arcs from the client one at a time, which are inserted into a list and written to the temporary file as soon as it reaches a length of 10 elements.

At the end of the iteration, a push is performed in any case if the last block is composed of less than 10 elements.

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
