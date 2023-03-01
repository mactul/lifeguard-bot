#include "str_utils.h"
#include <string.h>
#include <ctype.h>

#define is_ascii(c) (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || char_in_str(";,/?:@&=+$-_.!~*'()#%", c))

#define MAX_URL_SIZE 1024

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
int stristr(const char* string, const char* expr)
{
    int string_counter = 0;
    int expr_counter = 0;
    while(string[string_counter] != '\0')
    {
        if(tolower(string[string_counter]) == tolower(expr[0]))
        {
            while(expr[expr_counter] != '\0' && string[string_counter] != '\0' && tolower(string[string_counter]) == tolower(expr[expr_counter]))
            {
                expr_counter++;
                string_counter++;
            }
            if(expr[expr_counter] == '\0')
            {
                return string_counter;
            }
            expr_counter = 0;
        }
        string_counter++;
    }
    return -1;
}

char starts_with(const char* str, const char* ref)
{
    int i = 0;
    while(str[i] != '\0' && ref[i] != '\0' && str[i] == ref[i])
    {
        i++;
    }
    return ref[i] == '\0';
}

char starts_with_case_unsensitive(const char* str, const char* ref)
{
    int i = 0;
    while(str[i] != '\0' && ref[i] != '\0' && tolower(str[i]) == tolower(ref[i]))
    {
        i++;
    }
    return ref[i] == '\0';
}

char char_in_str(const char* str, char c)
{
    while(*str != '\0')
    {
        if(*str == c)
            return 1;
        
        str++;
    }
    return 0;
}

char* strtrim(char* str)
{
    while(!is_ascii(*str))
    {
        str++;
    }
    int i = strlen(str) - 1;
    while(!is_ascii(str[i]))
    {
        str[i] = '\0';
        i--;
    }

    return str;
}