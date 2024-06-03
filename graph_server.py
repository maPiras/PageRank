import sys, threading, logging, time, os
import concurrent.futures
import tempfile, subprocess
import struct, socket

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
        executor.shutdown()

def gestisci_connessione(connessione,addr):
    with tempfile.NamedTemporaryFile(mode='w', suffix=".mtx",delete=False,) as temp:
        scartati = 0
        
        
        buffer = list()
        
        data = connessione.recv(8)
        N = struct.unpack("!2i",data)[0]
        A = struct.unpack("!2i",data)[1]
        temp.write(f"{N} {N} {A}\n")
        
        for i in range(A):
            data = connessione.recv(8)
            From = struct.unpack("!2i",data)[0]
            To = struct.unpack("!2i",data)[1]
            
            if From<=0 or From>N or To<=0 or To>N:
                scartati += 1
                continue
            
            buffer.append(f"{From} {To}\n")
            
            if len(buffer) >= 10:
                for v in buffer:
                    temp.write(v)
                    
                buffer.clear()
        
        if(len(buffer) != 0):
            temp.writelines(buffer)
            buffer.clear()
        temp.seek(0)
        command = ['./pagerank',temp.name]
        esito = subprocess.run(command,capture_output=True)
            
        
        if(esito.returncode != 0):    
            connessione.sendall(struct.pack("!i",esito.returncode))
            connessione.sendall(struct.pack("!i",len(esito.stderr)))
            connessione.sendall(esito.stderr)
        else:   
            connessione.sendall(struct.pack("!i",0))
            connessione.sendall(struct.pack("!i",len(esito.stdout)))
            connessione.sendall(esito.stdout)  
            
        
        logging.info(f"Nodi del grafo: {N} Scartati: {scartati} Validi: {A-scartati}")
        logging.info(f"File temporaneo: {temp.name} Exit code: {esito.returncode}")
            
        
main()