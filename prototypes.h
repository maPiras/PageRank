#include "xerrori.h"
#define BUFFSIZE 30

void help();

typedef struct{
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
pthread_mutex_t *vmutex;
double *x;
double *y;
double *xnext;
int *index;
pthread_mutex_t *imutex;
int term;
int dump;
}dati_calcolatori;

void* tbody_scrittura(void *arg);
void* tbody_calcolo(void *arg);
grafo* crea_grafo(const char*,int);
void inserisci(grafo*, arco);
void nodes_dead_end_valid_arcs(grafo*);
double *pagerank(grafo*, double, double, int, int, int*);