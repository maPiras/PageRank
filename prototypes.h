#include "xerrori.h"
#define BUFFSIZE 30

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
    pthread_mutex_t *imutex;
    pthread_cond_t *cv;
    int index;
}vector_cond;

typedef struct{
    pthread_mutex_t *amutex;
    pthread_cond_t *cv;
    int aux_index;
}aux;

typedef struct{
    pthread_mutex_t *tmutex;
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
vector_cond *vector_cond;
terminated *terminated_cond;
aux *aux_cond;
}dati_calcolatori;

void* tbody_scrittura(void *arg);
void* tbody_calcolo(void *arg);
grafo* crea_grafo(const char*,int);
void inserisci(grafo*, arco);
void nodes_dead_end_valid_arcs(grafo*);
double *pagerank(grafo*, double, double, int, int, int*);
void help();
