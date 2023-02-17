#include "connexion_data.h"

enum DESTINATIONS {
    DEST_VIRUS_CHECKER,
    DEST_SCRAPPING_UNIT
};

typedef struct serverqueue_el {
    char ip[22];
    uint64_t port;
    double avg_time;
    struct serverqueue_el* next;
} ServerQueue_el;

typedef struct linksqueue_el {
    Links_data data;
    char destination;
    struct linksqueue_el* next;
} LinksQueue_el;

void queue_add_server(ServerQueue_el** pproot, char* ip, uint64_t port, double avg_time, pthread_mutex_t* pmutex);
char queue_next_server(ServerQueue_el** pproot, ServerQueue_el* pel, pthread_mutex_t* pmutex);

void queue_add_links(LinksQueue_el** pproot, Links_data* data, char destination, pthread_mutex_t* pmutex);
char queue_next_links(LinksQueue_el** pproot, LinksQueue_el* pel, pthread_mutex_t* pmutex);