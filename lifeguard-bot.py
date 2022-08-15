import socket
from connexion_data import *

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b"Hello, world")
    data = s.recv(1024)

if len(data) == 1 and data[0] == OK:
    print("data sended succesfuly")
else:
    print("connexion interrupted before the end of the transfert")