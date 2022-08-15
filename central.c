#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "connexion_data.h"

int main()
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;

    struct sockaddr_in my_addr, peer_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    my_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    my_addr.sin_port = htons(SERVER_PORT);

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
 
    if (bind(server, (struct sockaddr*) &my_addr, sizeof(my_addr)) == 0)
        printf("Binded Correctly\n");
    else
        printf("Unable to bind\n");
         
    if (listen(server, 3) == 0)
        printf("Listening ...\n");
    else
        printf("Unable to listen\n");
     
    socklen_t addr_size;
    addr_size = sizeof(struct sockaddr_in);

    while (1)
    {
        int acc = accept(server, (struct sockaddr*) &peer_addr, &addr_size);
        int n;
        char url[MAX_URL_SIZE];
        char returned = OK;
        
        printf("Connection Established\n");
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
     
        // "ntohs(peer_addr.sin_port)" function is
        // for finding port number of client
        printf("\tIP   : %s\n\tPORT : %d\n", ip, ntohs(peer_addr.sin_port));

        n = recv(acc, url, sizeof(url), 0);
        url[n] = '\0';

        printf("\n%s\n", url);

        send(acc, &returned, sizeof(char), 0);

        close(acc);
    }
}