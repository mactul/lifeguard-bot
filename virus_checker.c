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
#include "cmp_hash.h"
#include "connexion_data.h"
#include "central_credentials.h"
#include <sys/stat.h>


void request_db(void)
{
    struct sockaddr_in my_addr, my_addr1;
    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct stat st;
    Cmp_hash hash;

    Conn_infos info_data;

    FILE* fptr;
    char mode[] = "ab";

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
    

    info_data.what = REQUEST_DB;
    info_data.password = CENTRAL_PASSWORD;
    if(stat("db2.bin", &st) != -1)
    {
        info_data.port_or_dbpos = st.st_size;
    }
    else
    {
        info_data.port_or_dbpos = 0;
        mode[0] = 'w';
    }
    info_data.ip = 0; // not required for request_db
    
    send(client, &info_data, sizeof(info_data), 0);  // send the data to the server

    hash.size = 1;

    if ((fptr = fopen("db2.bin", mode)) == NULL)
    {
        printf("Error! opening file");
        return;
    }

    while(hash.size != 0)
    {
        int n;
        char returned;
        n = recv(client, &hash, sizeof(hash), 0);
        while(n != sizeof(hash))
        {
            returned = TRANSFERT_ERROR;
            send(client, &returned, sizeof(char), 0);
            n = recv(client, &hash, sizeof(hash), 0);
        }
        if(hash.size != 0)
        {
            fwrite(&hash, sizeof(hash), 1, fptr);
        }
        returned = TRANSFERT_OK;
        send(client, &returned, sizeof(char), 0);
    }
    fclose(fptr);

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

    while (1)
    {
        int acc = accept(server, (struct sockaddr*) &peer_addr, &addr_size);
        if(acc != -1)
        {
            printf("Connection Established\n");
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
        
            // "ntohs(peer_addr.sin_port)" function is
            // for finding port number of client
            printf("\tIP   : %s\n\tPORT : %d\n", ip, ntohs(peer_addr.sin_port));

            close(acc);
        }
    }
}

int main()
{
    request_db();

    listen_links();
    
    return 0;
}