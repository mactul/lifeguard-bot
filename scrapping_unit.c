#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "utils.h"
#include "requests.h"
#include "easy_tcp_tls.h"
#include "connexion_data.h"
#include "central_credentials.h"

#define BUFFER_SIZE 2048

#define MACHINE_IP "127.0.0.1"
static int machine_port;

#define HTML_A_TAG_STR "<a "
#define HREF_STR "href="

static uint64_t number_of_files_downloaded = 0;
static double avg_time_seconds = 0;

char is_char_of_url(char c)
{
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || char_in_str(";,/?:@&=+$-_.!~*'()#%", c);
}

int get_base_url(char* host, char* url)
{
    int i = 0;
    int n = 0;
    while(*url != '\0' && ((*url != '/' && *url != '?' && *url != '#') || n < 2))
    {
        if(*url == '/')
            n++;
        host[i] = *url;
        i++;
        url++;
    }
    host[i] = '\0';
    return i;
}

char contains_relevant_words(const char* url)
{
    char* relevant_words[] = {
        "download",
        "file",
        "game",
        "window",
        "mac",
        "linux",
        NULL
    };

    int i = 0;
    while(relevant_words[i] != NULL)
    {
        if(stristr(url, relevant_words[i]) != -1)
            return 1;
        i++;
    }
    return 0;
}

char req_is_file(RequestsHandler* handler)
{
    const char* content_type = req_get_header_value(handler, "content-type");
    const char* content_disposition = req_get_header_value(handler, "content-disposition");

    req_display_headers(handler);
    
    return (content_type != NULL && stristr(content_type, "text/html") == -1) || (content_disposition != NULL && stristr(content_disposition, "attachment") != -1);
}

char is_file(const char* url)
{
    RequestsHandler* handler;

    handler = req_head(url, "");

    if(handler == NULL)
    {
        // head is not working, try get to be sure
        handler = req_get(url, "");
        if(handler == NULL)
            return 0;  // server unreachable
    }

    char result = req_is_file(handler);

    req_close_connection(&handler);

    return result;
}

char is_relevant_url(const char* url, char priority)
{
    char host[MAX_URL_SIZE];
    char extension[MAX_EXTENSION_SIZE];

    url_slicer(url, host, extension);

    return !is_image_extension(extension) && !trusted_host(host) &&(!is_web_extension(extension) || (contains_relevant_words(url) && priority < MAX_DEPTH));
}

char check_href(char c)
{
    static int i = 0;

    if(i < strlen(HREF_STR) && tolower(c) != HREF_STR[i])
    {
        i = 0;
        return 0;
    }

    i++;

    if(i == strlen(HREF_STR)+1)
    {
        i = 0;
        return 1;
    }
    return 0;
}

char update_url(char* url, char c, const char* reference_url)
{
    static int i = 0;
    static int j = -1;
    static char href_passed = 0;
    char quotes_just_opened = 0;

    if(i >= MAX_URL_SIZE || (i < strlen(HTML_A_TAG_STR) && tolower(c) != HTML_A_TAG_STR[i]) || (j == -1 && c == '>'))
    {
        i = 0;
        j = -1;
        href_passed = 0;
        return 0;
    }

    i++;

    if(check_href(c))
    {
        href_passed = 1;
    }

    if(href_passed && j == -1 && (c == '"' || c == '\''))
    {
        quotes_just_opened = 1;
        j = 0;
    }
    
    if(j >= 0 && is_char_of_url(c))
    {
        url[j] = c;
        j++;
    }
    else if(j >= 0 && !quotes_just_opened)
    {
        url[j] = '\0';
        i = 0;
        j = -1;
        href_passed = 0;
        
        retrieve_absolute_url(url, reference_url);
        if(char_in_str(url, '.') && strlen(url) >= 11)
        {
            // coherent url
            return 1;
        }
    }
    return 0;
}


void send_url(Links_data link_data, char* url)
{
    SocketHandler* client;

    client = socket_ssl_client_init(CENTRAL_IP, UNKNOWN_LINKS_PORT, NULL);
    
    if(client == NULL)
    {
        socket_print_last_error();
        return;
    }
    link_data.priority += 1;
    link_data.password = socket_ntoh64(CENTRAL_PASSWORD);

    strcpy(link_data.url, url);
    
    socket_send(client, (char*)&link_data, sizeof(link_data), 0);  // send the data to the server

    socket_close(&client);
}

void find_urls(Links_data* data)
{
    RequestsHandler* handler;
    char buffer[BUFFER_SIZE+1];
    char url[MAX_URL_SIZE];
    int n;

    data->url[MAX_URL_SIZE-1] = '\0';

    handler = req_get(data->url, "Connection: close\r\n");

    if(handler == NULL)
        return;
    
    if(req_get_status_code(handler) >= 400)
        return;

    if(req_is_file(handler))
    {
        data->file_certified = 1;
        send_url(*data, data->url);

        req_close_connection(&handler);

        return;
    }
    
    while((n = req_read_output_body(handler, (char*)buffer, BUFFER_SIZE)) > 0)
    {
        for(int i = 0; i < n; i++)
        {
            if(update_url(url, buffer[i], data->url))
            {
                if(is_relevant_url(url, data->priority))
                {
                    data->file_certified = is_file(url);
                    printf("url found: %s\n", url);
                    send_url(*data, url);
                }
                printf("unrelevant url elapsed\n");
            }
        }
    }

    req_close_connection(&handler);
}

char send_ready(char* ip, uint64_t port)
{
    SocketHandler* client;
    Conn_infos info_data;

    printf("send ready\n");

    client = socket_ssl_client_init(CENTRAL_IP, INFOS_PORT, NULL);
    
    if(client == NULL)
    {
        socket_print_last_error();
        return 0;
    }
    info_data.what = SCR_UNIT_READY;
    info_data.password = socket_ntoh64(CENTRAL_PASSWORD);
    info_data.port = socket_ntoh64(port);
    info_data.avg_time = avg_time_seconds;
    strcpy(info_data.ip, ip);
    
    socket_send(client, (char*)&info_data, sizeof(info_data), 0);  // send the data to the server

    socket_close(&client);

    return 1;
}

void listen_links(void)
{
    SocketHandler* server;

    server = socket_ssl_server_init(MACHINE_IP, machine_port, 1, "cert.pem", "key.pem");

    if(server == NULL)
    {
        socket_print_last_error();
        return;
    }

    unsigned int seconds_to_wait = 1;
    while(!send_ready(MACHINE_IP, machine_port))
    {
        sleep(seconds_to_wait);
        seconds_to_wait += 2;
    }

    while (1)
    {
        Links_data data;
        SocketHandler* client = socket_accept(server, NULL);
        if(client != NULL)
        {
            socket_recv(client, (char*)&data, sizeof(data), 0);
            //data.url[n-sizeof(char)-3*sizeof(uint64_t)] = '\0';

            data.password = socket_ntoh64(data.password);

            socket_close(&client);

            printf("%s\n", data.url);

            time_t start = time(NULL);

            if(data.password == CENTRAL_PASSWORD)
            {
                find_urls(&data);
            }

            avg_time_seconds = (avg_time_seconds*number_of_files_downloaded + difftime(time(NULL), start))/(number_of_files_downloaded+1);
            number_of_files_downloaded++;

            unsigned int seconds_to_wait = 1;
            while(!send_ready(MACHINE_IP, machine_port))
            {
                sleep(seconds_to_wait);
                seconds_to_wait++;
            }
        }
        else
        {
            socket_print_last_error();
        }
    }
}


int main(int argc, char* argv[])
{
    if(argc == 2)
    {
        machine_port = atoi(argv[1]);
    }
    else
    {
        machine_port = 15793;
    }

    socket_start();

    listen_links();
    
    socket_cleanup();
    
    return 0;
}