#include <stdlib.h>
#include <string.h>
#include "str_utils.h"
#include "url_utils.h"

void url_slicer(const char* url, char* host, char* extension)
{
    int i = 0;
    int n = 0;

    while(*url != '\0' && n < 2)
    {
        if(*url == '/')
            n++;
        url++;
    }
    while(*url != '\0' && *url != '/' && *url != '?' && *url != '#')
    {
        if(*url == '/')
            n++;
        host[i] = *url;
        i++;
        url++;
    }
    if(*url == '/')
        n++;
    
    host[i] = '\0';
    while(*url != '\0' && *url != '.' && *url != '?' && *url != '#')
    {
        url++;
    }

    extension[0] = '\0';

    if(*url == '.' && n > 2)
    {
        i = 0;
        char is_extension = 1;
        while(*url != '\0' && *url != '?' && *url != '#')
        {
            extension[i] = *url;
            if(*url == '.')
            {
                is_extension = 1;
                i = 0;
            }
            else if(*url == '/')
            {
                is_extension = 0;
                i = 0;
            }
            url++;
            if(is_extension)
                i++;
        }
        extension[i] = '\0';
    }
}

char trusted_host(const char* host)
{
    char* trusted_hosts[] = {
        "youtube.com",
        "www.youtube.com",
        "www.youtu.be",
        "youtu.be",
        "twitter.com",
        "www.twitter.com",
        "github.com",
        "stackoverflow.com",
        NULL
    };

    int i = 0;
    while(trusted_hosts[i] != NULL)
    {
        if(strcasecmp(trusted_hosts[i], host) == 0)
            return 1;
        i++;
    }
    return 0;
}

void retrieve_absolute_url(char* url, const char* reference_url)
{
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
        int n_slash = 0;
        while(reference_url[end_of_path] != '\0' && reference_url[end_of_path] != '?' && reference_url[end_of_path] != '#')
        {
            if(reference_url[end_of_path] == '/')
                n_slash++;
            end_of_path++;
        }
        k = end_of_path;
        if(n_slash > 2)
        {
            char is_file = 0;
            while(reference_url[k] != '/')
            {
                if(reference_url[k] == '.')
                    is_file = 1;
                k--;
            }
            if(is_file)
                end_of_path = k;
        }
        
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
    }
}