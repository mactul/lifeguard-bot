#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "easy_tcp_tls.h"
#include "database.h"
#include "connexion_data.h"
#include "central_credentials.h"

// This will be replaced by argv or by an automatic algorithm
#define MACHINE_IP "127.0.0.1"
#define MACHINE_PORT 15792

char send_ready(char* ip, uint64_t port)
{
    SocketHandler* client;
    Conn_infos info_data;

    printf("send ready\n");

    client = socket_ssl_client_init(CENTRAL_IP, INFOS_PORT, NULL);
    
    if(client == NULL)
    {
        socket_print_last_error();
        return 0;
    }
    info_data.what = VCHECKER_READY;
    info_data.password = socket_ntoh64(CENTRAL_PASSWORD);
    info_data.port = socket_ntoh64(port);
    strcpy(info_data.ip, ip);
    
    socket_send(client, (char*)&info_data, sizeof(info_data), 0);  // send the data to the server

    socket_close(&client);

    return 1;
}

void send_audit_to_bot(Audit* paudit)
{
    SocketHandler* client;

    client = socket_client_init(BOT_IP, AUDIT_PORT);

    if(client == NULL)
    {
        socket_print_last_error();
        return;
    }
    
    socket_send(client, (char*)paudit, sizeof(Audit), 0);  // send the data to the server

    socket_close(&client);
}

void listen_links(void)
{
    SocketHandler* server;

    server = socket_ssl_server_init(MACHINE_IP, MACHINE_PORT, 1, "cert.pem", "key.pem");

    if(server == NULL)
    {
        socket_print_last_error();
        return;
    }

    unsigned int seconds_to_wait = 1;
    while(!send_ready(MACHINE_IP, MACHINE_PORT))
    {
        sleep(seconds_to_wait);
        seconds_to_wait += 2;
    }

    while (1)
    {
        Links_data data;
        SocketHandler* client = socket_accept(server, NULL);
        if(client != NULL)
        {
            Cmp_hash hash;
            
            socket_recv(client, (char*)&data, sizeof(data), 0);
            //data.url[n-sizeof(char)-3*sizeof(uint64_t)] = '\0';

            data.channel_id = socket_ntoh64(data.channel_id);
            data.message_id = socket_ntoh64(data.message_id);
            data.password = socket_ntoh64(data.password);

            socket_close(&client);

            if(data.password == CENTRAL_PASSWORD)
            {
                Audit audit;
                printf("%llu %s\n", data.message_id, data.url);

                cmp_create_hash_from_url(&hash, data.url);

                printf("%d\n", hash.size);

                audit.channel_id = socket_ntoh64(data.channel_id);
                audit.message_id = socket_ntoh64(data.message_id);
                audit.password = socket_ntoh64(CENTRAL_PASSWORD);
                audit.p = best_malware_correspondance(&hash);

                if(audit.p <= 0.5)
                {
                    audit.p = 0.0;
                }
                else
                {
                    audit.p = (audit.p - 0.5)*2;
                }

                printf("p: %f\n", audit.p);

                send_audit_to_bot(&audit);
            }


            unsigned int seconds_to_wait = 1;
            while(!send_ready(MACHINE_IP, MACHINE_PORT))
            {
                sleep(seconds_to_wait);
                seconds_to_wait++;
            }
        }
        else
        {
            socket_print_last_error();
        }
    }
}

int main()
{
    socket_start();

    listen_links();
    
    socket_cleanup();
    
    return 0;
}