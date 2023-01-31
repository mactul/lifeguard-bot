#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>
#include "easy_tcp_tls.h"
#include "central_credentials.h"
#include "database.h"
#include "queue.h"


/***********************************************************/
/* /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\  */
/* Do not forget to fix the little-endian/big-endian issue */
/* /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\  */
/***********************************************************/

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

char send_to_v_checker(ServerQueue_el* v_checker_addr, Links_data* pdata)
{
    SocketHandler* client;
    
    client = socket_ssl_client_init(v_checker_addr->ip, v_checker_addr->port, NULL);
    if(client == NULL)
    {
        socket_print_last_error();
        return 0;
    }
    pdata->channel_id = socket_ntoh64(pdata->channel_id);
    pdata->message_id = socket_ntoh64(pdata->message_id);
    pdata->password = socket_ntoh64(pdata->password);

    socket_send(client, (char*)pdata, sizeof(Links_data), 0);  // send the data to the server

    socket_close(&client, 0);

    return 1;
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

        if(!send_to_v_checker(&server_el, &(links_el.data)))
        {
            // echec
            queue_add_links(&v_checkers_links_queue, &(links_el.data), &links_mutex);
            sem_post(&v_links_sem);
        }
    }
}

void* unknown_links_gestion(void* arg)
{
    SocketHandler* server;

    server = socket_ssl_server_init(CENTRAL_IP, UNKNOWN_LINKS_PORT, 1, "cert.pem", "key.pem");

    if(server == NULL)
    {
        socket_print_last_error();
        return NULL;
    }

    while (1)
    {
        SocketHandler* client = socket_accept(server, NULL);
        if(client != NULL)
        {
            Links_data data;
            char extension[MAX_EXTENSION_SIZE];
            char returned = TRANSFERT_OK;

            socket_recv(client, (char*)&data, sizeof(data), 0);
            data.password = socket_ntoh64(data.password);
            data.channel_id = socket_ntoh64(data.channel_id);
            data.message_id = socket_ntoh64(data.message_id);

            printf("%d %llu\n", data.priority, data.password);

            if(data.password == CENTRAL_PASSWORD)
            {
                printf("\n%llu %d %llu %llu %s\n", sizeof(data), data.priority, data.channel_id, data.message_id, data.url);

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
                else if(strcmp(extension, ".png") != 0 && strcmp(extension, ".jpg") != 0
                    && strcmp(extension, ".jpeg") != 0 && strcmp(extension, ".bmp") != 0
                    && strcmp(extension, ".webp") != 0 && strcmp(extension, ".gif") != 0)
                {
                    // it's a file and not an image
                    queue_add_links(&v_checkers_links_queue, &data, &links_mutex);
                    sem_post(&v_links_sem);
                }

                socket_send(client, &returned, sizeof(char), 0);
            }

            socket_close(&client, 0);
        }
        else
        {
            socket_print_last_error();
        }
    }
}

void* conn_infos_gestion(void* arg)
{
    SocketHandler* server = socket_ssl_server_init(CENTRAL_IP, INFOS_PORT, 1, "cert.pem", "key.pem");

    if(server == NULL)
    {
        socket_print_last_error();
        return NULL;
    }

    while (1)
    {
        SocketHandler* client = socket_accept(server, NULL);
        if(client != NULL)
        {
            Conn_infos infos_data;

            socket_recv(client, (char*)(&infos_data), sizeof(infos_data), 0);

            infos_data.password = socket_ntoh64(infos_data.password);
            infos_data.port = socket_ntoh64(infos_data.port);

            printf("%llu %llu\n", infos_data.password, infos_data.port);

            if(infos_data.password == CENTRAL_PASSWORD)
            {
                switch(infos_data.what)
                {
                    case READY:
                        queue_add_server(&v_checkers_servers_queue, infos_data.ip, infos_data.port, &server_mutex);
                        sem_post(&v_server_sem);
                        break;
                    default:
                        break;
                }
            }

            socket_close(&client, 1);
        }
        else
        {
            socket_print_last_error();
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

    socket_start();

    sem_init(&v_server_sem, 0, 0);
    sem_init(&v_links_sem, 0, 0);
    
    pthread_create(&unknown_links_thread, NULL, *unknown_links_gestion, NULL);

    pthread_create(&conn_infos_thread, NULL, *conn_infos_gestion, NULL);

    pthread_create(&links_attribution_thread, NULL, *links_attribution, NULL);

    pthread_join(unknown_links_thread, NULL); // infinite loop in thread

    socket_cleanup();
}