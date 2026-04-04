#!/usr/bin/env python3
"""
graph_client.py
Client for the PageRank graph server.

Reads one or more Matrix Market (.mtx) files, sends each file's edges to the
server over a TCP connection, and prints the result returned by the server.
Each file is handled in its own thread so multiple files are transmitted
concurrently.

Usage:
    python3 graph_client.py <file1> [file2 ... fileN]
"""

import sys
import struct
import socket
import threading

HOST = "127.0.0.1"
PORT = 51112


def send_file(filename: str) -> None:
    """Open a TCP connection to the server, stream the edges from `filename`,
    then receive and print the PageRank result."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((HOST, PORT))

        with open(filename, "r") as f:
            for line in f:
                # Skip Matrix Market comment lines.
                if line[0] == "%":
                    continue

                parts = line.split()

                # Header line: "rows cols num_edges"  -> send (rows, num_edges)
                # Edge line:   "from to"              -> send (from, to)
                if len(parts) == 3:
                    sock.send(struct.pack("!2i", int(parts[0]), int(parts[2])))
                elif len(parts) == 2:
                    sock.send(struct.pack("!2i", int(parts[0]), int(parts[1])))

        # Receive the response: [return_code (4B)] [msg_len (4B)] [msg (msg_len B)]
        return_code = struct.unpack("!i", sock.recv(4))[0]
        msg_len     = struct.unpack("!i", sock.recv(4))[0]
        message     = sock.recv(msg_len).decode()

        print(f"{filename} exit code: {return_code}")
        for line in message.splitlines():
            if line:
                print(f"{filename} {line}")
        print(f"{filename} done")


def main(files: list[str]) -> None:
    """Launch one thread per file and wait for all of them to finish."""
    threads = [threading.Thread(target=send_file, args=(fn,)) for fn in files]
    for t in threads:
        t.start()
    for t in threads:
        t.join()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <file1> [file2 ... fileN]", file=sys.stderr)
        sys.exit(1)

    main(sys.argv[1:])
