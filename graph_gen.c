#include "xerrori.h"
#include "prototypes.h"

grafo* crea_grafo(const char *infile,int T){
  FILE* fd = xfopen(infile, "r", QUI);
  if(fd == NULL) xtermina("Errore apertura infile\n", QUI);

  char* line = NULL;
  size_t len = 0;
  
  while(getline(&line,&len,fd) != -1){
    if(line[0]=='%') continue;
    break;
  }

  int N1;
  int N2;
  int num_archi;
  
  sscanf(line, "%d %d %d", &N1,&N2,&num_archi);

  if(N1 != N2) xtermina("Dimensione non valida", QUI);

  grafo *graph = malloc(sizeof(grafo));
  graph->out = malloc(N1*sizeof(int));
  graph->in = malloc(N1*sizeof(inmap));
  graph->N = N1;
  
  for(int i=0; i<N1; i++)
    graph->out[i] = 0;

  for(int i=0; i<N1; i++)
    graph->in[i] = NULL;

  arco Buffer[BUFFSIZE];

  pthread_mutex_t bmutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t gmutex = PTHREAD_MUTEX_INITIALIZER;
  
  sem_t items;
  sem_t free_slots;
  
  xsem_init(&items, 0, 0, QUI);
  xsem_init(&free_slots, 0, BUFFSIZE, QUI);
  
  int pbindex = 0;
  int cbindex = 0;

  pthread_t t[T];
  dati_consumatori d[T];

  for(int i=0; i<T; i++){
    d[i].bmutex = &bmutex;
    d[i].free = &free_slots;
    d[i].items = &items;
    d[i].buffer = Buffer;
    d[i].cbindex = &cbindex;
    d[i].graph = graph;
    d[i].gmutex = &gmutex;

    xpthread_create(&t[i], NULL, &tbody_scrittura, &d[i], QUI);
  }
  
  
  while(getline(&line,&len,fd) != -1){
    arco arch;
    sscanf(line, "%d %d",&arch.from,&arch.to);

    if(arch.from > graph->N || arch.to > graph->N)
    continue;

    arch.from --;
    arch.to --;
    xsem_wait(&free_slots, QUI);
    xpthread_mutex_lock(&bmutex, QUI);
    Buffer[pbindex % BUFFSIZE] = arch;
    pbindex += 1;
    xpthread_mutex_unlock(&bmutex, QUI);
    xsem_post(&items, QUI);
  }

  arco fine;
  fine.from = -1;
  fine.to = -1;
  
  for(int i=0; i<T; i++){
    xsem_wait(&free_slots, QUI);
    xpthread_mutex_lock(&bmutex, QUI);
    Buffer[pbindex % BUFFSIZE] = fine;
    pbindex += 1;
    xpthread_mutex_unlock(&bmutex, QUI);
    xsem_post(&items, QUI);
  }

  for(int i=0; i<T; i++)
    xpthread_join(t[i], NULL, QUI);

  xpthread_mutex_destroy(&bmutex, QUI);
  xpthread_mutex_destroy(&gmutex, QUI);

  xsem_destroy(&items, QUI);
  xsem_destroy(&free_slots, QUI);
  
  fclose(fd);

  free(line);

  return graph;
}