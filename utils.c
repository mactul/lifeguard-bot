#include "utils.h"
#include <string.h>
#include <ctype.h>

void int_to_string(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do
    {
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse_string(s);
}

void reverse_string(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void bytescpy(char* dest, const char* src, int n)
{
    int i = 0;
    while(i < n)
    {
        dest[i] = src[i];
        i++;
    }
}

/* return the position of the first occurence's end
this function is non case-sensitive */
int stristr(const char* string, const char* exp)
{
    int string_counter = 0;
    int exp_counter = 0;
    while(string[string_counter] != '\0')
    {
        if(tolower(string[string_counter]) == tolower(exp[0]))
        {
            while(exp[exp_counter] != '\0' && string[string_counter] != '\0' && tolower(string[string_counter]) == tolower(exp[exp_counter]))
            {
                exp_counter++;
                string_counter++;
            }
            if(exp[exp_counter] == '\0')
            {
                return string_counter;
            }
            exp_counter = 0;
        }
        string_counter++;
    }
    return -1;
}

char starts_with(char* str, const char* ref)
{
    int i = 0;
    while(str[i] != '\0' && ref[i] != '\0' && str[i] == ref[i])
    {
        i++;
    }
    return ref[i] == '\0';
}

char starts_with_case_unsensitive(char* str, const char* ref)
{
    int i = 0;
    while(str[i] != '\0' && ref[i] != '\0' && tolower(str[i]) == tolower(ref[i]))
    {
        i++;
    }
    return ref[i] == '\0';
}

char char_in_str(char* str, char c)
{
    while(*str != '\0')
    {
        if(*str == c)
            return 1;
        
        str++;
    }
    return 0;
}

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

char trusted_host(char* host)
{
    char* trusted_hosts[] = {
        "youtube.com",
        "www.youtube.com",
        "www.youtu.be",
        "youtu.be",
        "twitter.com",
        "www.twitter.com",
        "github.com",
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