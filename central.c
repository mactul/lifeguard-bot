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
#include <semaphore.h>
#include "central_credentials.h"
#include "database.h"
#include "queue.h"

#define DISCORD_ATTACHMENTS_START "https://cdn.discordapp.com/attachments/"

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t links_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t v_server_sem;
sem_t v_links_sem;

ServerQueue v_checkers_servers_queue;
LinksQueue v_checkers_links_queue;

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

void send_to_v_checker(ServerQueue_el* v_checker_addr, Links_data* pdata)
{
    struct sockaddr_in my_addr;
    int client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0)
        printf("Error in client creating\n");
    else
        printf("Client Created\n");
         
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = v_checker_addr->port;
    
    // This ip address is the server ip address
    my_addr.sin_addr.s_addr = v_checker_addr->ip;
     
    socklen_t addr_size = sizeof my_addr;
    int con = connect(client, (struct sockaddr*) &my_addr, sizeof my_addr);
    if (con == 0)
        printf("Client Connected\n");
    else
        printf("Error in Connection\n");
    
    
    send(client, pdata, sizeof(Links_data), 0);  // send the data to the server
}

void* links_attribution(void* arg)
{
    while(1)
    {
        LinksQueue_el links_el;
        ServerQueue_el server_el;

        sem_wait(&v_links_sem);
        sem_wait(&v_server_sem);

        queue_next_server(&v_checkers_servers_queue, &server_el, &server_mutex);
        queue_next_links(&v_checkers_links_queue, &links_el, &links_mutex);

        send_to_v_checker(&server_el, &(links_el.data));
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
         
    if (listen(server, 1) == 0)
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
            Links_data data;
            char extension[MAX_EXTENSION_SIZE];
            char returned = TRANSFERT_OK;

            printf("%d\n", acc);
            
            printf("Connection Established\n");
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
        
            // "ntohs(peer_addr.sin_port)" function is
            // for finding port number of client
            printf("\tIP   : %s\n\tPORT : %d\n", ip, ntohs(peer_addr.sin_port));

            n = recv(acc, &data, sizeof(data), 0);
            data.url[n-sizeof(char)-sizeof(uint64_t)] = '\0';

            printf("\n%d %d %llu %s\n", sizeof(data), data.priority, data.message_id, data.url);

            get_extension(data.url, extension);

            printf("%s\n", extension);

            if((extension[0] == '\0' || strcmp(extension, ".html") == 0
                || strcmp(extension, ".htm") == 0 || strcmp(extension, ".asp") == 0
                || strcmp(extension, ".php") == 0 || strcmp(extension, ".php3") == 0
                || strcmp(extension, ".shtm") == 0 || strcmp(extension, ".shtml") == 0
                || strcmp(extension, ".cfm") == 0 || strcmp(extension, ".cfml") == 0)
                && !startswith(data.url, DISCORD_ATTACHMENTS_START))
            {
                // it's a website
            }
            else
            {
                // it's a file
                queue_add_links(&v_checkers_links_queue, &data, &links_mutex);
                sem_post(&v_links_sem);
            }

            send(acc, &returned, sizeof(char), 0);

            close(acc);
        }
    }
}

void* conn_infos_gestion(void* arg)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;

    struct sockaddr_in my_addr, peer_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    my_addr.sin_addr.s_addr = inet_addr(CENTRAL_IP);
    my_addr.sin_port = htons(INFOS_PORT);

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
            Conn_infos infos_data;
            
            printf("Connection Established\n");
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
        
            // "ntohs(peer_addr.sin_port)" function is
            // for finding port number of client
            printf("\tIP   : %s\n\tPORT : %d\n", ip, ntohs(peer_addr.sin_port));

            recv(acc, &infos_data, sizeof(infos_data), 0);

            if(infos_data.password == CENTRAL_PASSWORD)
            {
                switch(infos_data.what)
                {
                    case REQUEST_DB: ;
                        FILE* fptr;
                        Cmp_hash hash;
                        char returned;

                        if ((fptr = fopen("db.bin","rb")) == NULL)
                        {
                            printf("Error! opening file");
                            return NULL;
                        }
                        fseek(fptr, infos_data.port_or_dbpos, SEEK_SET);
                        while(get_next_malware_hash(&hash, fptr))
                        {
                            returned = TRANSFERT_ERROR;
                            if(hash.size != 0)
                            {
                                while(returned != TRANSFERT_OK)
                                {
                                    send(acc, &hash, sizeof(hash), 0);
                                    recv(acc, &returned, sizeof(char), 0);
                                }
                            }
                        }
                        hash.size = 0;
                        send(acc, &hash, sizeof(hash), 0);

                        fclose(fptr);
                        break;
                    case READY:
                        queue_add_server(&v_checkers_servers_queue, infos_data.ip, infos_data.port_or_dbpos, &server_mutex);
                        sem_post(&v_server_sem);
                        break;
                    default:
                        break;
                }
            }

            close(acc);
        }
    }
}

int main()
{
    pthread_t unknown_links_thread;
    pthread_t conn_infos_thread;
    pthread_t links_attribution_thread;

    v_checkers_servers_queue.first = NULL;
    v_checkers_servers_queue.last = NULL;

    v_checkers_links_queue.first = NULL;
    v_checkers_links_queue.last = NULL;

    sem_init(&v_server_sem, 0, 0);
    sem_init(&v_links_sem, 0, 0);
    
    pthread_create(&unknown_links_thread, NULL, *unknown_links_gestion, NULL);

    pthread_create(&conn_infos_thread, NULL, *conn_infos_gestion, NULL);

    pthread_create(&links_attribution_thread, NULL, *links_attribution, NULL);

    pthread_join(unknown_links_thread, NULL); // infinite loop in thread
}