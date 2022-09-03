import ctypes
from pickle import REDUCE
import discord
import asyncio
from struct import unpack
from connexion_data import *
from central_credentials import *
from discord_credentials import TOKEN
import traceback

GREEN = 0x05f776
ORANGE = 0xff7700
RED = 0xff0000

def build_links_data(priority, password, channel_id, message_id, url):
    return bytes([priority])+bytes(ctypes.c_uint64(password))+bytes(ctypes.c_uint64(channel_id))+bytes(ctypes.c_uint64(message_id))+bytes(url, 'ascii')+bytes([0])

def decode_audit_data(audit):
    return unpack('QQQd', audit)


async def handle_audit_response(reader, writer):
    data = await reader.read(4*8) # sizeof(uin64_t)+sizeof(uin64_t)+sizeof(uin64_t)+sizeof(double)
    channel_id, message_id, password, p = decode_audit_data(data)

    print(message_id, password, p)
    channel = client.get_channel(channel_id)
    message = await channel.fetch_message(message_id)

    if p == 0.0:
        embed = discord.Embed(title="Antivirus Scanning Audit", description="We have not found any known virus that matches this file", color=GREEN)
    else:
        if p <= 0.75:
            color = ORANGE
        else:
            color = RED
        embed = discord.Embed(title="Antivirus Scanning Audit", description="We suspect that this file is a malware variant.\nConfidence: "+str(round(100*p, 2))+"%", color=color)
    await message.reply(embed=embed)

async def audit_server():
    while True:
        try:
            server = await asyncio.start_server(handle_audit_response, BOT_IP, AUDIT_PORT)
            
            addrs = ', '.join(str(sock.getsockname()) for sock in server.sockets)
            print(f'Serving on {addrs}')

            async with server:
                await server.serve_forever()
        except Exception as e:
            file = open("python_server_logs.txt", "a")
            traceback.print_exc(file=file)
            file.close()
            await asyncio.sleep(5)  # We have to wait a little bit before trying to reconnect
        

intents = discord.Intents.all()
client = discord.Client(intents=intents)

@client.event
async def on_message(message:discord.Message):
    if message.author == client.user:
        return
        
    for attachment in message.attachments:
        try:
            reader, writer = await asyncio.open_connection(CENTRAL_IP, UNKNOWN_LINKS_PORT)
            
            writer.write(build_links_data(1, CENTRAL_PASSWORD, message.channel.id, message.id, attachment.url))
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
    


@client.event
async def on_ready():
    print("bot logged in as:")
    print(client.user.name)
    print(client.user.id)
    print("---------")

    client.loop.create_task(audit_server())
    
    print("released")

client.run(TOKEN)
