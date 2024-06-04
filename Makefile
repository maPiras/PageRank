CC = gcc
CFLAGS = -std=c11 -Wall -g -O -pthread
LIBS = -lm -lrt -pthread

SRCS = main.c graph_gen.c pagerank.c auxfunctions.c errors/xerrori.c
OBJS = main.o graph_gen.o pagerank.o auxfunctions.o errors/xerrori.o

all: pagerank

# Regola per creare l'eseguibile
pagerank: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Regola per creare i file oggetto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regola per pulire i file generati
clean:
	rm -f $(OBJS) pagerank