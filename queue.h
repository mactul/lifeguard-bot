#include "connexion_data.h"

typedef struct serverqueue_el {
    uint64_t ip;
    uint64_t port;

    struct serverqueue_el* newer;
} ServerQueue_el;

typedef struct linksqueue_el {
    Links_data data;

    struct linksqueue_el* newer;
} LinksQueue_el;

typedef struct serverqueue {
    ServerQueue_el* first;
    ServerQueue_el* last;
} ServerQueue;

typedef struct linksqueue {
    LinksQueue_el* first;
    LinksQueue_el* last;
} LinksQueue;

void queue_add_server(ServerQueue* pqueue, uint64_t ip, uint64_t port, pthread_mutex_t* pmutex);
char queue_next_server(ServerQueue* pqueue, ServerQueue_el* pel, pthread_mutex_t* pmutex);

void queue_add_links(LinksQueue* pqueue, Links_data* data, pthread_mutex_t* pmutex);
char queue_next_links(LinksQueue* pqueue, LinksQueue_el* pel, pthread_mutex_t* pmutex);