import ctypes
from pickle import REDUCE
import discord
import asyncio
from struct import unpack
from connexion_data import *
from central_credentials import *
from discord_credentials import TOKEN
import traceback
import ssl

GREEN = 0x05f776
ORANGE = 0xff7700
RED = 0xff0000


LITTLE_ENDIAN = bytes(ctypes.c_short(1))[0] == 1

def to_big_endian(b_array):
    if LITTLE_ENDIAN:
        return bytes(reversed(b_array))
    return b_array

def build_links_data(priority, password, channel_id, message_id, url):
    bytes(reversed(bytes(ctypes.c_uint64(12))))
    return bytes([priority])+to_big_endian(bytes(ctypes.c_uint64(password)))+to_big_endian(bytes(ctypes.c_uint64(channel_id)))+to_big_endian(bytes(ctypes.c_uint64(message_id)))+bytes(url, 'ascii')+bytes([0])

def decode_audit_data(audit):
    print(audit)
    print(len(audit))
    channel_id, message_id, password, p, name = unpack('QQQd256s', audit)
    channel_id = bytes(ctypes.c_uint64(channel_id))
    message_id = bytes(ctypes.c_uint64(message_id))
    password = bytes(ctypes.c_uint64(password))
    name = name.decode().split('\0')[0]
    
    return int.from_bytes(channel_id, "big"), int.from_bytes(message_id, "big"), int.from_bytes(password, "big"), p, name


async def handle_audit_response(reader, writer):
    data = await reader.read(4*8+256) # sizeof(uin64_t)+sizeof(uin64_t)+sizeof(uin64_t)+sizeof(double)+sizeof(char[256])
    writer.close()
    channel_id, message_id, password, p, name = decode_audit_data(data)

    if password == CENTRAL_PASSWORD:
        print(message_id, password, p, name)
        channel = client.get_channel(channel_id)
        message = await channel.fetch_message(message_id)

        if p == 0.0:
            embed = discord.Embed(title="Antivirus Scanning Audit", description="We have not found any known virus that matches this file:\n\t"+name, color=GREEN)
        else:
            if p <= 0.75:
                color = ORANGE
            else:
                color = RED
            embed = discord.Embed(title="Antivirus Scanning Audit", description="We suspect that this file ("+name+") is a malware variant.\nConfidence: "+str(round(100*p, 2))+"%", color=color)
        await message.reply(embed=embed)

async def audit_server():
    while True:
        try:
            server = await asyncio.start_server(handle_audit_response, BOT_IP, AUDIT_PORT)
            
            addrs = ', '.join(str(sock.getsockname()) for sock in server.sockets)
            print(f'Serving on {addrs}')

            async with server:
                await server.serve_forever()
        except:
            await asyncio.sleep(5)  # We have to wait a little bit before trying to reconnect


def is_char_of_url(c):
    return ('A' <= c and c <= 'Z') or ('a' <= c and c <= 'z') or ('0' <= c and c <= '9') or c in ";,/?:@&=+$-_.!~*'()#"

def extract_urls(content):
    urls = []
    i = 0
    while i < len(content):
        if content[i:i+7] == "http://" or content[i:i+8] == "https://":
            a = i
            while i < len(content) and content[i] != '/':
                i += 1
            i += 2
            while i < len(content) and is_char_of_url(content[i]):
                i += 1
            urls.append(content[a: i])
        i += 1

    return urls


async def send_link(channel_id, message_id, url):
    try:
        ssl_ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ssl_ctx.check_hostname = False
        ssl_ctx.verify_mode = ssl.CERT_NONE
        reader, writer = await asyncio.open_connection(CENTRAL_IP, UNKNOWN_LINKS_PORT, ssl=ssl_ctx)
        
        writer.write(build_links_data(1, CENTRAL_PASSWORD, channel_id, message_id, url))
        await writer.drain()

        data = await reader.read(1)

        if len(data) == 1 and data[0] == TRANSFERT_OK:
            print("data sended succesfuly")
        else:
            print("connexion interrupted before the end of the transfert")
        
        writer.close()
        await writer.wait_closed()
    except Exception as e:
        file = open("python_server_logs.txt", "a")
        traceback.print_exc(file=file)
        file.close()

intents = discord.Intents.all()
client = discord.Client(intents=intents)

@client.event
async def on_message(message:discord.Message):
    if message.author == client.user:
        return
        
    for attachment in message.attachments:
        print(attachment)
        await send_link(message.channel.id, message.id, attachment.url)
    
    for url in extract_urls(message.content):
        await send_link(message.channel.id, message.id, url)


@client.event
async def on_ready():
    print("bot logged in as:")
    print(client.user.name)
    print(client.user.id)
    print("---------")

    client.loop.create_task(audit_server())
    
    print("released")

client.run(TOKEN)
