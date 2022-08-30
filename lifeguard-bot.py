import ctypes
import socket
import discord
from struct import unpack
from connexion_data import *
from threading import Thread
from central_credentials import *
from discord_credentials import TOKEN

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

intents = discord.Intents.all()
client = discord.Client(intents=intents)

@client.event
async def on_message(message:discord.Message):
    if message.author == client.user:
        return
        
    for attachment in message.attachments:
        print(attachment.url)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((CENTRAL_IP, UNKNOWN_LINKS_PORT))
            s.sendall(build_links_data(1, CENTRAL_PASSWORD, message.id, attachment.url))
            data = s.recv(1)

            if len(data) == 1 and data[0] == TRANSFERT_OK:
                print("data sended succesfuly")
            else:
                print("connexion interrupted before the end of the transfert")
    


@client.event
async def on_ready():
    print("bot logged in as:")
    print(client.user.name)
    print(client.user.id)
    print("---------")

    Thread(target=server).start()

client.run(TOKEN)
