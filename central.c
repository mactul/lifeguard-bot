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
#include "utils.h"


/***********************************************************/
/* /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\  */
/* Do not forget to fix the little-endian/big-endian issue */
/* /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\  */
/***********************************************************/

#define DISCORD_ATTACHMENTS_START "https://cdn.discordapp.com/attachments/"

pthread_mutex_t v_server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t links_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_server_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t v_server_sem;
sem_t links_sem;
sem_t s_server_sem;

ServerQueue v_checkers_servers_queue;
ServerQueue scrp_unit_servers_queue;
LinksQueue links_queue;


char send_to_analysis(ServerQueue_el* server_addr, Links_data* pdata)
{
    SocketHandler* client;
    
    client = socket_ssl_client_init(server_addr->ip, server_addr->port, NULL);
    if(client == NULL)
    {
        socket_print_last_error();
        return 0;
    }
    pdata->channel_id = socket_ntoh64(pdata->channel_id);
    pdata->message_id = socket_ntoh64(pdata->message_id);
    pdata->password = socket_ntoh64(pdata->password);

    socket_send(client, (char*)pdata, sizeof(Links_data), 0);  // send the data to the server

    socket_close(&client);

    return 1;
}

void* links_attribution(void* arg)
{
    while(1)
    {
        LinksQueue_el links_el;
        ServerQueue_el server_el;

        sem_wait(&links_sem);

        queue_next_links(&links_queue, &links_el, &links_mutex);

        if(links_el.destination == DEST_VIRUS_CHECKER)
        {
            sem_wait(&v_server_sem);
            queue_next_server(&v_checkers_servers_queue, &server_el, &v_server_mutex);
        }
        else
        {
            sem_wait(&s_server_sem);
            queue_next_server(&scrp_unit_servers_queue, &server_el, &s_server_mutex);
        }
        if(!send_to_analysis(&server_el, &(links_el.data)))
        {
            // echec
            queue_add_links(&links_queue, &(links_el.data), links_el.destination, &links_mutex);
            sem_post(&links_sem);
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
            char host[MAX_URL_SIZE];
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

                url_slicer(data.url, host, extension);

                printf("%s\n", extension);

                if(is_web_extension(extension) && !starts_with(data.url, DISCORD_ATTACHMENTS_START))
                {
                    // it's a website
                    if(data.priority <= MAX_DEPTH && !trusted_host(host))
                    {
                        queue_add_links(&links_queue, &data, DEST_SCRAPPING_UNIT, &links_mutex);
                        sem_post(&links_sem);
                    }
                }
                else if(!is_image_extension(extension))
                {
                    // it's a file and not an image
                    queue_add_links(&links_queue, &data, DEST_VIRUS_CHECKER, &links_mutex);
                    sem_post(&links_sem);
                }

                socket_send(client, &returned, sizeof(char), 0);
            }

            socket_close(&client);
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

            printf("%s\n %llu %llu\n", infos_data.ip, infos_data.password, infos_data.port);

            if(infos_data.password == CENTRAL_PASSWORD)
            {
                switch(infos_data.what)
                {
                    case VCHECKER_READY:
                        queue_add_server(&v_checkers_servers_queue, infos_data.ip, infos_data.port, &v_server_mutex);
                        sem_post(&v_server_sem);
                        break;
                    case SCR_UNIT_READY:
                        queue_add_server(&scrp_unit_servers_queue, infos_data.ip, infos_data.port, &s_server_mutex);
                        sem_post(&s_server_sem);
                        break;
                    default:
                        break;
                }
            }

            socket_close(&client);
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

    scrp_unit_servers_queue.first = NULL;
    scrp_unit_servers_queue.last = NULL;

    links_queue.first = NULL;
    links_queue.last = NULL;

    socket_start();

    sem_init(&v_server_sem, 0, 0);
    sem_init(&s_server_sem, 0, 0);
    sem_init(&links_sem, 0, 0);
    
    pthread_create(&unknown_links_thread, NULL, *unknown_links_gestion, NULL);

    pthread_create(&conn_infos_thread, NULL, *conn_infos_gestion, NULL);

    pthread_create(&links_attribution_thread, NULL, *links_attribution, NULL);

    pthread_join(unknown_links_thread, NULL); // infinite loop in thread

    socket_cleanup();
}