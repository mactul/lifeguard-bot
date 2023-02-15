
#define CENTRAL_IP "127.0.0.1"
#define BOT_IP "127.0.0.1"

#define UNKNOWN_LINKS_PORT 12732
#define INFOS_PORT 12733
#define AUDIT_PORT 12734

#define MAX_URL_SIZE 1024
#define MAX_EXTENSION_SIZE 32

#define TRANSFERT_OK 1
#define TRANSFERT_ERROR 2

#define MAX_DEPTH 2

enum what {
    VCHECKER_READY,
    SCR_UNIT_READY
};

typedef struct __attribute__((__packed__)) links_data {
    char priority;
    uint64_t password;
    uint64_t channel_id;
    uint64_t message_id;
    char url[MAX_URL_SIZE];
} Links_data;


typedef struct __attribute__((__packed__)) conn_infos {
    char what;
    uint64_t password;
    uint64_t port;
    char ip[22];
} Conn_infos;

typedef struct __attribute__((__packed__)) audit {
    uint64_t channel_id;
    uint64_t message_id;
    uint64_t password;
    double p;
} Audit;