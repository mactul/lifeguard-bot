#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "cmp_hash.h"
#include "connexion_data.h"
#include "central_credentials.h"
#include <sys/stat.h>


void request_db(void)
{
    struct sockaddr_in my_addr, my_addr1;
    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct stat st;
    Cmp_hash hash;

    Conn_infos info_data;

    FILE* fptr;
    char mode[] = "ab";

    if (client < 0)
        printf("Error in client creating\n");
    else
        printf("Client Created\n");
         
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(INFOS_PORT);
    
    // This ip address is the server ip address
    my_addr.sin_addr.s_addr = inet_addr(CENTRAL_IP);
     
    socklen_t addr_size = sizeof my_addr;
    int con = connect(client, (struct sockaddr*) &my_addr, sizeof my_addr);
    if (con == 0)
        printf("Client Connected\n");
    else
        printf("Error in Connection\n");
    

    info_data.what = REQUEST_DB;
    info_data.password = CENTRAL_PASSWORD;
    if(stat("db2.bin", &st) != -1)
    {
        info_data.port_or_dbpos = st.st_size;
    }
    else
    {
        info_data.port_or_dbpos = 0;
        mode[0] = 'w';
    }
    info_data.ip = 0; // not required for request_db

    printf("%d\n", info_data.port_or_dbpos);
    
    send(client, &info_data, sizeof(info_data), 0);  // send the data to the server

    hash.size = 1;

    if ((fptr = fopen("db2.bin", mode)) == NULL)
    {
        printf("Error! opening file");
        return;
    }

    while(hash.size != 0)
    {
        int n;
        char returned;
        n = recv(client, &hash, sizeof(hash), 0);
        while(n != sizeof(hash))
        {
            returned = TRANSFERT_ERROR;
            send(client, &returned, sizeof(char), 0);
            n = recv(client, &hash, sizeof(hash), 0);
        }
        if(hash.size != 0)
        {
            fwrite(&hash, sizeof(hash), 1, fptr);
        }
        returned = TRANSFERT_OK;
        send(client, &returned, sizeof(char), 0);
    }
    fclose(fptr);

}


int main()
{
    request_db();
    
    return 0;
}