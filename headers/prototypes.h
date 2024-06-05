#include <limits.h>

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

grafo *g;
pthread_mutex_t *bmutex;
pthread_mutex_t *gmutex;

sem_t *items;
sem_t *free;

int *cbindex;
arco *buffer;

}dati_consumatori;

typedef struct coppia_indice{
    int indice;
    double rank;
}coppia_indice;

typedef struct{
grafo *g;

double *x;
double *y;
double *y_aux;
double *xnext;

double *St;
double *St_new;

double term1;
double dump;

int *iter;
double *errore;

vector_cond *vector_cond; //Condition variable per consumare l'indice
terminated *terminated_cond; //Condition variable per incrementare il contatore dei thread che hanno terminato il calcolo
pthread_mutex_t *aux; //Mutex ausiliaria per aggiornare il fattore St ausiliario (new)
coppia_indice* massimo; //Struct per tenere traccia del nodo con rank massimo

}dati_calcolatori;


typedef struct handler_data{
    coppia_indice* massimo;
    int *iterazione;
    pthread_mutex_t* mutex;
}handler_data;

grafo* crea_grafo(const char*,int);
void inserisci(grafo*, arco);
void* tbody_scrittura(void *arg);

double *pagerank(grafo*, double, double, int, int, int*);
void* tbody_calcolo(void *arg);

void nodes_dead_end_valid_arcs(grafo*);

int compare(const void* a,const void* b); //Funzione usata per 
void help(); //Stampa messaggio help

void deallocate(grafo*); //Funzione ausiliaria di deallocazione del grafo
void *handler_body(void *); //Corpo del gestore segnali