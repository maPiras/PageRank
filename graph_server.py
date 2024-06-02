import sys, threading, logging, time, os
import concurrent.futures
import socket
import tempfile
import struct

HOST = "127.0.0.1"  
PORT = 51112

logging.basicConfig(filename="server" + '.log',
                    level=logging.DEBUG, datefmt='%H:%M:%S',
                    format='%(asctime)s - %(levelname)s - %(message)s')

def main(host=HOST,port=PORT):
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)            
            s.bind((host, port))
            s.listen()
            with concurrent.futures.ThreadPoolExecutor() as executor:
                while True:
                    print("In attesa di un client...")
                    conn, addr = s.accept()
                    executor.submit(gestisci_connessione, conn,addr)
        except KeyboardInterrupt:
            pass
            
        print("Bye dal server\n")
        s.shutdown(socket.SHUT_RDWR)


def gestisci_connessione(connessione,addr):
    with tempfile.NamedTemporaryFile(mode='a', suffix=".mtx") as temp:
        v = connessione.recv(8)
        N = struct.unpack("!2i",v)[0]
        A = struct.unpack("!2i",v)[1]
        temp.write(f"{N} {N} {A}\n")
        print(f"{N} {A}")
        Arch = connessione.recv(8)
        From = struct.unpack("!2i",Arch)[0]
        To = struct.unpack("!2i",Arch)[1]
main()