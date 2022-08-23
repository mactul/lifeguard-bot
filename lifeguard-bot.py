import ctypes
import socket
from connexion_data import *

def build_bin_data(priority, message_id, url):
    return bytes([priority])+bytes(ctypes.c_uint64(message_id))+bytes(url, 'ascii')

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(build_bin_data(12, 1011632414604406946, "https://truc.com/test.php"))
    data = s.recv(1024)

if len(data) == 1 and data[0] == OK:
    print("data sended succesfuly")
else:
    print("connexion interrupted before the end of the transfert")