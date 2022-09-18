#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include "database.h"
#include "connexion_data.h"
#include "central_credentials.h"
#include <sys/stat.h>



void send_ready(struct sockaddr_in addr)
{
    struct sockaddr_in my_addr;
    int client = socket(AF_INET, SOCK_STREAM, 0);
    
    Conn_infos info_data;


    if (client < 0)
        printf("Error in client creating\n");
    else
        printf("Client Created\n");
         
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(INFOS_PORT);
    
    // This ip address is the server ip address
    my_addr.sin_addr.s_addr = inet_addr(CENTRAL_IP);
     
    socklen_t addr_size = sizeof my_addr;
    int con = connect(client, (struct sockaddr*) &my_addr, sizeof my_addr);
    if (con == 0)
        printf("Client Connected\n");
    else
        printf("Error in Connection\n");
    

    info_data.what = READY;
    info_data.password = CENTRAL_PASSWORD;
    info_data.port = addr.sin_port;
    info_data.ip = addr.sin_addr.s_addr;
    
    send(client, &info_data, sizeof(info_data), 0);  // send the data to the server

}

void send_audit_to_bot(Audit* paudit)
{
    struct sockaddr_in my_addr;
    int client = socket(AF_INET, SOCK_STREAM, 0);
    
    Conn_infos info_data;


    if (client < 0)
        printf("Error in client creating\n");
    else
        printf("Client Created\n");
         
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(AUDIT_PORT);
    
    // This ip address is the server ip address
    my_addr.sin_addr.s_addr = inet_addr(BOT_IP);
     
    socklen_t addr_size = sizeof my_addr;
    int con = connect(client, (struct sockaddr*) &my_addr, sizeof my_addr);
    if (con == 0)
        printf("Client Connected\n");
    else
        printf("Error in Connection\n");
    
    
    send(client, paudit, sizeof(Audit), 0);  // send the data to the server

}

void listen_links(void)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;
    struct ifreq ifr;
    struct sockaddr_in my_addr, peer_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(server, SIOCGIFADDR, &ifr);
    
    my_addr.sin_addr.s_addr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    my_addr.sin_port = htons(0);  // any available port

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
 
    if (bind(server, (struct sockaddr*) &my_addr, sizeof(my_addr)) == 0)
        printf("Binded Correctly\n");
    else
        printf("Unable to bind\n");
        
    if (listen(server, 3) == 0)
    {
        printf("Listening ...\n");
    }
    else
        printf("Unable to listen\n");

    socklen_t addr_size;
    addr_size = sizeof(struct sockaddr_in);

    getsockname(server, (struct sockaddr*) &my_addr, &addr_size);

    printf("%s %d\n", inet_ntoa(my_addr.sin_addr), ntohs(my_addr.sin_port));

    send_ready(my_addr);

    while (1)
    {
        int n;
        Links_data data;
        int acc = accept(server, (struct sockaddr*) &peer_addr, &addr_size);
        if(acc != -1)
        {
            Cmp_hash hash;
            printf("Connection Established\n");
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
        
            // "ntohs(peer_addr.sin_port)" function is
            // for finding port number of client
            printf("\tIP   : %s\n\tPORT : %d\n", ip, ntohs(peer_addr.sin_port));

            n = recv(acc, &data, sizeof(data), 0);
            //data.url[n-sizeof(char)-3*sizeof(uint64_t)] = '\0';

            close(acc);

            if(data.password == CENTRAL_PASSWORD)
            {
                Audit audit;
                printf("%llu %s\n", data.message_id, data.url);

                cmp_create_hash_from_url(&hash, data.url);

                printf("%d\n", hash.size);

                audit.channel_id = data.channel_id;
                audit.message_id = data.message_id;
                audit.password = CENTRAL_PASSWORD;
                audit.p = best_malware_correspondance(&hash);

                if(audit.p <= 0.5)
                {
                    audit.p = 0.0;
                }
                else
                {
                    audit.p = (audit.p - 0.5)*2;
                }

                printf("%f\n", audit.p);

                send_audit_to_bot(&audit);
            }


            send_ready(my_addr);
        }
    }
}

int main()
{
    listen_links();
    
    return 0;
}