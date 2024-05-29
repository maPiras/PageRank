# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

SRCS = main.c graph_gen.c pagerank.c auxfunctions.c xerrori.c
OBJS = main.o graph_gen.o pagerank.o auxfunction.o xerrori.o

pagerank: $(OBJS)
	$(CC) $^ $(LDLIBS) -o $<

$(OBJS): $(SRCS) xerrori.h prototypes.h
	$(CC) $(CFLAGS) -c $<

gcc -std=c11 -Wall -g -O -pthread -lm -lrt -pthread *.c *.h -o pagerank
