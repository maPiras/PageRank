# PageRank

A multi-threaded C implementation of the PageRank algorithm, with a Python TCP client/server interface for remote graph submission.

## Overview

This project implements the classic PageRank algorithm (as used by Google) using a parallelised power-iteration method. The core engine is written in C and leverages POSIX threads for concurrent rank computation. A Python server wraps the binary to accept graphs over a TCP socket, enabling remote execution.

### Algorithm

The iterative formula applied at each step is:

```
x_new[i] = (1 - d) / N  +  d * Σ_j ( x[j] / out(j) )  +  (d / N) * St
```

where:
- `d` — damping factor (default `0.9`)
- `N` — total number of nodes
- `St` — sum of ranks of dangling nodes (nodes with no outgoing edges)

Iteration stops when the L1 norm `||x_new - x||₁ < ε` or the maximum iteration count is reached.

## Project Structure

```
.
├── headers/
│   ├── prototypes.h    # Shared type definitions and function prototypes
│   └── errcheck.h      # Checked POSIX wrapper declarations
├── src/
│   ├── main.c          # Entry point: CLI argument parsing and output
│   ├── graph_gen.c     # Graph construction from .mtx files (producer-consumer)
│   ├── pagerank.c      # Multi-threaded PageRank power-iteration
│   ├── auxfunctions.c  # Thread bodies, graph utilities, signal handler
│   └── errcheck.c      # Checked POSIX wrapper implementations
├── test/
│   └── 21archi.mtx     # Sample Matrix Market graph for testing
├── graph_client.py     # Python TCP client: sends .mtx files to the server
├── graph_server.py     # Python TCP server: invokes the pagerank binary
├── makefile
└── LICENSE
```

## Building

```bash
make
```

This produces the `pagerank` executable in the project root. Requires GCC with C11 support and POSIX thread/math/realtime libraries (`-lpthread -lm -lrt`).

To remove build artefacts:

```bash
make clean
```

## Usage

### Command-line binary

```
pagerank [-k K] [-m M] [-d D] [-e E] [-t T] <infile>
```

| Flag | Description | Default |
|------|-------------|---------|
| `-k K` | Number of top-ranked nodes to display | `3` |
| `-m M` | Maximum number of iterations | `100` |
| `-d D` | Damping factor | `0.9` |
| `-e E` | Convergence threshold (L1 norm) | `1e-7` |
| `-t T` | Number of worker threads | `3` |
| `infile` | Input graph in Matrix Market (`.mtx`) format | *(required)* |

**Example:**

```bash
./pagerank -k 5 -t 4 test/21archi.mtx
```

### Python server/client

Start the server (must be run from the directory containing the `pagerank` binary):

```bash
python3 graph_server.py
```

Send one or more `.mtx` files from the client (files are transmitted concurrently):

```bash
python3 graph_client.py test/21archi.mtx
```

The server listens on `127.0.0.1:51112` by default. Results and diagnostics are logged to `server.log`.

## Input Format

The tool expects graphs in [Matrix Market](https://math.nist.gov/MatrixMarket/formats.html) coordinate format (`.mtx`). Comment lines start with `%`. The first non-comment line is the header:

```
<rows> <cols> <num_entries>
```

followed by one edge per line:

```
<from> <to>
```

Indices are 1-based. Self-loops and duplicate edges are silently discarded.

## Runtime Signals

While the binary is running, you can send signals to the process to observe progress:

| Signal | Effect |
|--------|--------|
| `SIGUSR1` | Print the current iteration number and the highest-ranked node to stderr |
| `SIGTERM` | Gracefully terminate the computation |

```bash
kill -USR1 <pid>
```

## License

See [LICENSE](LICENSE).
