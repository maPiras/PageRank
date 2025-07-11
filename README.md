# Implementazione dell'algoritmo pagerank in C

## Descrizione del progetto  
Il progetto rappresenta la consegna del terzo appello dell'esame di laboratorio II.
Fornisce l'implementazione del celebre algoritmo di page ranking (google).
La struttura principale è in C ma vengono forniti anche un client e server in python per interfacciarsi con tale strumento.

### Struttura

```text
├── graph_client.py
├── graph_server.py
├── headers
│   ├── prototypes.h
│   └── xerrori.h
├── makefile
└── src
    ├── auxfunctions.c
    ├── graph_gen.c
    ├── main.c
    ├── pagerank.c
    └── xerrori.c
```

## Building

La compilazione è agevolata dal makefile.  
Eseguire make dall'interno della directory principale e usare gli script python per interfacciarsi con lo strumento.
