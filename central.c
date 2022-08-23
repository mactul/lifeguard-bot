#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "connexion_data.h"

#define DISCORD_ATTACHMENTS_START "https://cdn.discordapp.com/attachments/"

char startswith(char* str, char* occ)
{
    while(*str == *occ && *str != '\0' && *occ != '\0')
    {
        str++;
        occ++;
    }
    return *occ == '\0';
}

void get_extension(char* url, char* extension)
{
    int i = strlen(url) - 1;
    while(url[i] != '.' && url[i] != '/' && i > 0)
    {
        i--;
    }
    if(url[i] == '/')
    {
        extension[0] = '\0';
    }
    else
    {
        int j = i;
        while(j > 1 && url[j] != '/')
        {
            j--;
        }
        if(url[j-1] == '/') // there is 2 /, it's http(s)://
            extension[0] = '\0';
        else
            strcpy(extension, &(url[i]));
    }
}

void* unknown_links_gestion(void* arg)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;

    struct sockaddr_in my_addr, peer_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    my_addr.sin_addr.s_addr = inet_addr(CENTRAL_IP);
    my_addr.sin_port = htons(UNKNOWN_LINKS_PORT);

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

    while (1)
    {
        int acc = accept(server, (struct sockaddr*) &peer_addr, &addr_size);
        if(acc != -1)
        {
            int n;
            char url[MAX_URL_SIZE];
            char extension[MAX_EXTENSION_SIZE];
            char returned = OK;

            printf("%d\n", acc);
            
            printf("Connection Established\n");
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
        
            // "ntohs(peer_addr.sin_port)" function is
            // for finding port number of client
            printf("\tIP   : %s\n\tPORT : %d\n", ip, ntohs(peer_addr.sin_port));

            n = recv(acc, url, sizeof(url), 0);
            url[n] = '\0';

            printf("\n%s\n", url);

            get_extension(url, extension);

            printf("%s\n", extension);

            if(extension[0] == '\0' || strcmp(extension, ".html") == 0
                || strcmp(extension, ".htm") == 0 || strcmp(extension, ".asp") == 0
                || strcmp(extension, ".php") == 0 || strcmp(extension, ".php3") == 0
                || strcmp(extension, ".shtm") == 0 || strcmp(extension, ".shtml") == 0
                || strcmp(extension, ".cfm") == 0 || strcmp(extension, ".cfml") == 0
                || !startswith(url, DISCORD_ATTACHMENTS_START))
            {
                // it's a website
            }
            else
            {
                // it's a file
            }

            send(acc, &returned, sizeof(char), 0);

            close(acc);
        }
    }
}

int main()
{
    pthread_t unknown_links_thread;
    
    pthread_create (&unknown_links_thread, NULL, *unknown_links_gestion, NULL);

    pthread_join(unknown_links_thread, NULL); // infinite loop in thread

    printf("end\n");
}