import sys, threading
import socket
import struct

HOST = "127.0.0.1"  
PORT = 51112

def tbody(filename):
    with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
        s.connect((HOST,PORT))
        
        with open(filename,"r") as f:
            for line in f:
                if line[0] == '%': continue
                form_line =line.split(" ")
                if len(form_line) == 3:
                    s.send(struct.pack("!2i",int(form_line[0]),int(form_line[2])))
                elif len(form_line) == 2:
                    s.send(struct.pack("!2i",int(form_line[0]),int(form_line[1])))
            
            
def main(fileArr):
    threads = list()
    for fn in fileArr:
        t=threading.Thread(target=tbody, args=(fn,))
        t.start()
        threads.append(t)
    
    for tr in threads:
        tr.join()
    
    
    

if len(sys.argv) >= 2:
    main(sys.argv[1:])
else:
    print(f"Uso: {sys.argv[0]} file1 ... fileN\n")