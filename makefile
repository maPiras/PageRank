CC     = gcc
CFLAGS = -std=c11 -Wall -g -O2 -pthread
LIBS   = -lm -lrt -pthread

SRCS = src/main.c src/graph_gen.c src/pagerank.c src/auxfunctions.c src/errcheck.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: pagerank

# Link all object files into the final executable.
pagerank: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile each source file to an object file.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Remove generated object files and the executable.
clean:
	rm -f $(OBJS) pagerank
