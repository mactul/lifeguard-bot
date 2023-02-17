#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include "queue.h"

void queue_add_server(ServerQueue_el** pproot, char* ip, uint64_t port, double avg_time, pthread_mutex_t* pmutex)
{
    ServerQueue_el* pel;

    pel = (ServerQueue_el*) malloc(sizeof(ServerQueue_el));
    strcpy(pel->ip, ip);
    pel->port = port;
    pel->avg_time = avg_time;
    pel->next = NULL;

    pthread_mutex_lock(pmutex);

    if(*pproot == NULL)
    {
        *pproot = pel;
        goto UNLOCK;
    }

    if(avg_time < (*pproot)->avg_time)
    {
        pel->next = *pproot;
        *pproot = pel;
        goto UNLOCK;
    }

    ServerQueue_el* root = *pproot;
    while(root->next != NULL && avg_time >= root->next->avg_time)
    {
        root = root->next;
    }
    // root->next > pel
    ServerQueue_el* last_root_next = root->next;
    root->next = pel;
    pel->next = last_root_next;

UNLOCK:
    pthread_mutex_unlock(pmutex);
}

char queue_next_server(ServerQueue_el** pproot, ServerQueue_el* pel, pthread_mutex_t* pmutex)
{
    ServerQueue_el* ptemp;

    pthread_mutex_lock(pmutex);

    if(*pproot == NULL)
    {
        pthread_mutex_unlock(pmutex);
        return 0;
    }

    ptemp = *pproot;

    *pproot = (*pproot)->next;

    *pel = *ptemp;

    free(ptemp);

    pthread_mutex_unlock(pmutex);

    return 1;
}

void queue_add_links(LinksQueue_el** pproot, Links_data* data, char destination, pthread_mutex_t* pmutex)
{
    LinksQueue_el* pel;

    pel = (LinksQueue_el*) malloc(sizeof(LinksQueue_el));
    pel->data = *data;
    pel->destination = destination;
    pel->next = NULL;

    pthread_mutex_lock(pmutex);

    if(*pproot == NULL)
    {
        *pproot = pel;
        goto UNLOCK;
    }

    if(data->priority < (*pproot)->data.priority)
    {
        pel->next = *pproot;
        *pproot = pel;
        goto UNLOCK;
    }

    LinksQueue_el* root = *pproot;
    while(root->next != NULL && data->priority >= root->next->data.priority)
    {
        root = root->next;
    }
    // root->next > pel
    LinksQueue_el* last_root_next = root->next;
    root->next = pel;
    pel->next = last_root_next;

UNLOCK:
    pthread_mutex_unlock(pmutex);
}

char queue_next_links(LinksQueue_el** pproot, LinksQueue_el* pel, pthread_mutex_t* pmutex)
{
    LinksQueue_el* ptemp;

    pthread_mutex_lock(pmutex);

    if(*pproot == NULL)
    {
        pthread_mutex_unlock(pmutex);
        return 0;
    }

    ptemp = *pproot;

    *pproot = (*pproot)->next;

    *pel = *ptemp;

    free(ptemp);

    pthread_mutex_unlock(pmutex);

    return 1;
}