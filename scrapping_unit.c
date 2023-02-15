#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "utils.h"
#include "requests.h"
#include "easy_tcp_tls.h"
#include "connexion_data.h"
#include "central_credentials.h"

#define BUFFER_SIZE 2048

#define MACHINE_IP "127.0.0.1"
#define MACHINE_PORT 15793

#define HTML_A_TAG_STR "<a "
#define HREF_STR "href="

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

char contains_relevant_words(char* url)
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

char is_relevant_url(char* url, char priority)
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
        if(!starts_with_case_unsensitive(url, "https://") && !starts_with_case_unsensitive(url, "http://"))
        {
            char relative_url[MAX_URL_SIZE];
            int k = 0;
            int n_backward = 0;
            while(url[k] == '/' || url[k+1] == '/' || url[k+2] == '/')
            {
                if(url[k] == '.' && url[k+1] == '.' && url[k+2] == '/')
                {
                    n_backward++;
                    k += 3;
                }
                else
                {
                    k++;
                }
            }
            strcpy(relative_url, &(url[k]));

            int end_of_path = 0;
            while(reference_url[end_of_path] != '\0' && reference_url[end_of_path] != '?' && reference_url[end_of_path] != '#')
            {
                end_of_path++;
            }
            k = end_of_path;
            char is_file = 0;
            while(reference_url[k] != '/')
            {
                if(reference_url[k] == '.')
                    is_file = 1;
                k--;
            }
            if(is_file)
                end_of_path = k;
            
            end_of_path--;

            if(reference_url[end_of_path] == '/')
                end_of_path--;

            while(n_backward > 0)
            {
                if(reference_url[end_of_path] == '/')
                    n_backward--;
                end_of_path--;
            }
            strncpy(url, reference_url, end_of_path+1);
            url[end_of_path+1] = '/';
            url[end_of_path+2] = '\0';

            strcat(url, relative_url);

            printf("%s\n", url);
        }
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
    
    while((n = req_read_output_body(handler, (char*)buffer, BUFFER_SIZE)) > 0)
    {
        for(int i = 0; i < n; i++)
        {
            if(update_url(url, buffer[i], data->url))
            {
                if(is_relevant_url(url, data->priority))
                {
                    printf("url found: %s\n", url);
                    send_url(*data, url);
                }
                printf("unrelevant url elapsed\n");
            }
        }
    }

    req_close_connection(&handler);
}

void send_ready(char* ip, uint64_t port)
{
    SocketHandler* client;
    Conn_infos info_data;

    printf("send ready\n");

    client = socket_ssl_client_init(CENTRAL_IP, INFOS_PORT, NULL);
    
    if(client == NULL)
    {
        socket_print_last_error();
        return;
    }
    info_data.what = SCR_UNIT_READY;
    info_data.password = socket_ntoh64(CENTRAL_PASSWORD);
    info_data.port = socket_ntoh64(port);
    strcpy(info_data.ip, ip);
    
    socket_send(client, (char*)&info_data, sizeof(info_data), 0);  // send the data to the server

    socket_close(&client);
}

void listen_links(void)
{
    SocketHandler* server;

    server = socket_ssl_server_init(MACHINE_IP, MACHINE_PORT, 1, "cert.pem", "key.pem");

    if(server == NULL)
    {
        socket_print_last_error();
        return;
    }

    send_ready(MACHINE_IP, MACHINE_PORT);

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

            if(data.password == CENTRAL_PASSWORD)
            {
                find_urls(&data);
            }


            send_ready(MACHINE_IP, MACHINE_PORT);
        }
        else
        {
            socket_print_last_error();
        }
    }
}


int main()
{
    socket_start();

    listen_links();
    
    socket_cleanup();
    
    return 0;
}