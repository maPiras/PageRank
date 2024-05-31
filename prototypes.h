#define BUFFSIZE 30

#define TOP_NODES 3
#define MAX_ITERATIONS 100
#define DUMPING 0.9
#define MAX_ERROR 1.e-07
#define THREADS 3

typedef struct arco{
int from;
int to;
}arco;

typedef struct inmap{
int N;
struct inmap* next;
}inmap;

typedef struct{
int N;
int *out;
inmap **in;
}grafo;

typedef struct{
    pthread_mutex_t *mutex;
    pthread_cond_t *cv;
    int index;
}vector_cond;

typedef struct{
    pthread_mutex_t *mutex;
    pthread_cond_t *cv;
    int aux_index;
}aux;

typedef struct{
    pthread_mutex_t *mutex;
    pthread_cond_t *cv;
    int terminated;
}terminated;

typedef struct{
pthread_mutex_t *bmutex;
sem_t *items;
sem_t *free;
int *cbindex;
arco *buffer;
pthread_mutex_t *gmutex;
grafo *graph;
}dati_consumatori;

typedef struct{
grafo *graph;
double *x;
double *y;
double *xnext;
double term1;
double dump;
int *iter;
double *errore;
double *St;
double *St_new;
double *y_aux;
vector_cond *vector_cond;
terminated *terminated_cond;
pthread_mutex_t *aux;
coppia_indice* massimo;

}dati_calcolatori;

typedef struct coppia_indice{
    int indice;
    double rank;
}coppia_indice;

void* tbody_scrittura(void *arg);
void* tbody_calcolo(void *arg);
grafo* crea_grafo(const char*,int);
void inserisci(grafo*, arco);
void nodes_dead_end_valid_arcs(grafo*);
double *pagerank(grafo*, double, double, int, int, int*);
int compare(const void* a,const void* b);
void help();
void deallocate(grafo*);
void *handler_body(void *);