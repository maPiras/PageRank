import sys, threading, logging, time, os
import concurrent.futures
import tempfile, subprocess
import struct, socket

HOST = "127.0.0.1"  
PORT = 51112

logging.basicConfig(filename="server" + '.log',
                    level=logging.DEBUG, datefmt='%H:%M:%S',
                    format='%(asctime)s - %(levelname)s - %(message)s')


def gestisci_connessione(connessione,addr):
    with tempfile.NamedTemporaryFile(mode='a+', suffix=".mtx") as temp:
        buffer = list()
        
        data = connessione.recv(8)
        N = struct.unpack("!2i",data)[0]
        A = struct.unpack("!2i",data)[1]
        temp.write(f"{N} {N} {A}")
        
        print(f"Inserita prima linea A: {A}")
        
        for i in range(A):
            data = connessione.recv(8)
            From = struct.unpack("!2i",data)[0]
            To = struct.unpack("!2i",data)[1]
            
            if From > N | To < 1: continue
            
            if len(buffer) >= 10:
                for j in buffer:
                    temp.write(j)

                buffer.clear()
        
        if(len(buffer) != 0):
            for j in buffer:
                temp.write(j)

            buffer.clear()
        
        command = f"./pagerank {temp}"
        esito = subprocess.run(command,capture_output=True)
        if(esito.returncode != 0):
            
        
        

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
            


        
main()



def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks