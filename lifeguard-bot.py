import socket

HOST = "185.163.124.98"  # The server's hostname or IP address
PORT = 12754  # The port used by the server

OK = 0

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b"Hello, world")
    data = s.recv(1024)

if len(data) == 1 and data[0] == OK:
    print("data sended succesfuly")
else:
    print("connexion interrupted before the end of the transfert")