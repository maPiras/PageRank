#!/usr/bin/env python3
"""
graph_server.py
TCP server that accepts graph data, runs the PageRank binary, and streams back
the result.

Protocol (big-endian, all integers are 4 bytes):
  Client -> Server:
    [N (int)] [A (int)]       — number of nodes and edges (first message)
    [from (int)] [to (int)]   — one message per edge (A messages total)

  Server -> Client:
    [return_code (int)]       — exit code of the pagerank process
    [msg_len (int)]           — length of the following message in bytes
    [message (msg_len bytes)] — stdout on success, stderr on failure

The server writes the received graph to a temporary .mtx file, invokes the
`./pagerank` binary on it, and sends back the output.  Each connection is
handled in its own thread via a ThreadPoolExecutor.

Log output is written to server.log in the working directory.
"""

import os
import sys
import struct
import socket
import logging
import tempfile
import subprocess
import concurrent.futures

HOST = "127.0.0.1"
PORT = 51112

logging.basicConfig(
    filename="server.log",
    level=logging.DEBUG,
    datefmt="%H:%M:%S",
    format="%(asctime)s - %(levelname)s - %(message)s",
)


def handle_connection(conn: socket.socket, addr: tuple) -> None:
    """Receive a graph from `conn`, run PageRank, and send back the result."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".mtx", delete=False) as tmp:
        discarded = 0

        # First packet: (N, A) — graph dimensions and edge count.
        header  = conn.recv(8)
        N, A    = struct.unpack("!2i", header)
        tmp.write(f"{N} {N} {A}\n")

        # Receive A edge packets and buffer them for bulk writing.
        write_buffer = []
        for _ in range(A):
            data      = conn.recv(8)
            src, dst  = struct.unpack("!2i", data)

            # Discard out-of-range indices silently.
            if src <= 0 or src > N or dst <= 0 or dst > N:
                discarded += 1
                continue

            write_buffer.append(f"{src} {dst}\n")

            # Flush the buffer in chunks to avoid holding too much in memory.
            if len(write_buffer) >= 10:
                tmp.writelines(write_buffer)
                write_buffer.clear()

        # Flush any remaining edges.
        if write_buffer:
            tmp.writelines(write_buffer)

        tmp.flush()
        tmp_path = tmp.name

    # Run the pagerank binary on the temporary file.
    result = subprocess.run(["./pagerank", tmp_path], capture_output=True)

    if result.returncode != 0:
        # Send stderr on failure so the client can display the error.
        conn.sendall(struct.pack("!i", result.returncode))
        conn.sendall(struct.pack("!i", len(result.stderr)))
        conn.sendall(result.stderr)
    else:
        # Send stdout on success.
        conn.sendall(struct.pack("!i", 0))
        conn.sendall(struct.pack("!i", len(result.stdout)))
        conn.sendall(result.stdout)

    logging.info(
        "Graph nodes: %d  discarded edges: %d  valid edges: %d",
        N, discarded, A - discarded,
    )
    logging.info("Temp file: %s  exit code: %d", tmp_path, result.returncode)

    conn.close()


def main(host: str = HOST, port: int = PORT) -> None:
    """Start the TCP server and accept connections indefinitely."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        try:
            srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            srv.bind((host, port))
            srv.listen()
            print(f"Server listening on {host}:{port}")

            with concurrent.futures.ThreadPoolExecutor() as pool:
                while True:
                    print("Waiting for a client...")
                    conn, addr = srv.accept()
                    pool.submit(handle_connection, conn, addr)

        except KeyboardInterrupt:
            pass

        print("Server shutting down.")
        srv.shutdown(socket.SHUT_RDWR)


if __name__ == "__main__":
    main()
