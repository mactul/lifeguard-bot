import ctypes
import socket
from struct import unpack
from threading import Thread
from connexion_data import *
from central_credentials import *

def build_links_data(priority, password, message_id, url):
    return bytes([priority])+bytes(ctypes.c_uint64(password))+bytes(ctypes.c_uint64(message_id))+bytes(url, 'ascii')

def decode_audit_data(audit):
    return unpack('QQd', audit)

def server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((BOT_IP, AUDIT_PORT))
        s.listen()
        while True:
            print("cc")
            conn, addr = s.accept()
            with conn:
                print(f"Connected by {addr}")
                data = conn.recv(3*8) # sizeof(uin64_t)+sizeof(uin64_t)+sizeof(double)
                message_id, password, p = decode_audit_data(data)

                print(message_id, password, p)

th = Thread(target=server)
th.start()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((CENTRAL_IP, UNKNOWN_LINKS_PORT))
    s.sendall(build_links_data(1, CENTRAL_PASSWORD, 1011632414604406946, "https://cdn.discordapp.com/attachments/575715281234690048/1012823945965228064/IMG_2306.png"))
    data = s.recv(1024)

if len(data) == 1 and data[0] == TRANSFERT_OK:
    print("data sended succesfuly")
else:
    print("connexion interrupted before the end of the transfert")

th.join()