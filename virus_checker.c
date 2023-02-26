#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "easy_tcp_tls.h"
#include "cmp_hash.h"
#include "database.h"
#include "connexion_data.h"
#include "central_credentials.h"

// This will be replaced by argv or by an automatic algorithm
#define MACHINE_IP "127.0.0.1"
static int machine_port;

static uint64_t number_of_files_downloaded = 0;
static double avg_time_seconds = 0;


void get_name_from_url(char* name, const char* url)
{
    int i = strlen(url);
    while(url[i] != '/')
    {
        i--;
    }
    i++;
    while(url[i] != '\0')
    {
        *name = url[i];
        name++;
        i++;
    }
    name[i] = '\0';
}

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
    info_data.avg_time = avg_time_seconds;
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

void resend_url(Links_data link_data)
{
    SocketHandler* client;

    client = socket_ssl_client_init(CENTRAL_IP, UNKNOWN_LINKS_PORT, NULL);
    
    if(client == NULL)
    {
        socket_print_last_error();
        return;
    }
    link_data.priority += 1;
    link_data.password = socket_ntoh64(CENTRAL_PASSWORD);
    link_data.channel_id = socket_ntoh64(link_data.channel_id);
    link_data.message_id = socket_ntoh64(link_data.message_id);
    
    socket_send(client, (char*)&link_data, sizeof(link_data), 0);  // send the data to the server

    socket_close(&client);
}

void listen_links(void)
{
    SocketHandler* server;

    server = socket_ssl_server_init(MACHINE_IP, machine_port, 1, "cert.pem", "key.pem");

    if(server == NULL)
    {
        socket_print_last_error();
        return;
    }

    unsigned int seconds_to_wait = 1;
    while(!send_ready(MACHINE_IP, machine_port))
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
                printf("%llu %s\n", (unsigned long long) data.message_id, data.url);

                time_t start = time(NULL);

                int error_code = cmp_create_hash_from_url(&hash, data.url);

                if(error_code == OK)
                {
                    printf("%d\n", hash.size);

                    audit.channel_id = socket_ntoh64(data.channel_id);
                    audit.message_id = socket_ntoh64(data.message_id);
                    audit.password = socket_ntoh64(CENTRAL_PASSWORD);
                    audit.p = best_malware_correspondance(&hash);
                    memset(audit.name, 0, sizeof(audit.name));
                    get_name_from_url(audit.name, data.url);

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
                else if(error_code == UNKNOW_ERROR)
                {
                    resend_url(data);
                }

                avg_time_seconds = (avg_time_seconds*number_of_files_downloaded + difftime(time(NULL), start))/(number_of_files_downloaded+1);
                number_of_files_downloaded++;

                printf("time avg: %.2f\n", avg_time_seconds);
            }


            unsigned int seconds_to_wait = 1;
            while(!send_ready(MACHINE_IP, machine_port))
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

int main(int argc, char* argv[])
{
    if(argc == 2)
    {
        machine_port = atoi(argv[1]);
    }
    else
    {
        machine_port = 15792;
    }

    socket_start();

    listen_links();
    
    socket_cleanup();
    
    return 0;
}