#include "connexion_data.h"

enum DESTINATIONS {
    DEST_VIRUS_CHECKER,
    DEST_SCRAPPING_UNIT
};

typedef struct serverqueue_el {
    char ip[22];
    uint64_t port;

    struct serverqueue_el* newer;
} ServerQueue_el;

typedef struct linksqueue_el {
    Links_data data;
    char destination;
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

void queue_add_server(ServerQueue* pqueue, char* ip, uint64_t port, pthread_mutex_t* pmutex);
char queue_next_server(ServerQueue* pqueue, ServerQueue_el* pel, pthread_mutex_t* pmutex);

void queue_add_links(LinksQueue* pqueue, Links_data* data, char destination, pthread_mutex_t* pmutex);
char queue_next_links(LinksQueue* pqueue, LinksQueue_el* pel, pthread_mutex_t* pmutex);