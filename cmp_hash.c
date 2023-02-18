#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include "cmp_hash.h"
#include "requests.h"

#define MAX_U32 4294967295


void _update_hash(Cmp_hash* phash, unsigned char* buffer, int n, uint32_t* last_viewed, uint32_t* max_gap, uint32_t index)
{
    /*
    Internal function, used to update the hash with the data of the buffer
    */
    uint32_t i = 0;  // i is uint32_t to avoid implicit conversions
    while(i < n)
    {
        if(max_gap[buffer[i]] < index + i - last_viewed[buffer[i]])
        {
            max_gap[buffer[i]] = index + i - last_viewed[buffer[i]];
            phash->data_gap[buffer[i]] = index + i;
        }
        last_viewed[buffer[i]] = index + i;
        (phash->data_occ[buffer[i]])++;
        i++;
    }
}

double corresponding_rate(double a, double b)
{
    if(a + b == 0)
    {
        return -1;
    }
    return 1.0 - abs(a - b)/(a+b);
}

enum status_codes cmp_create_hash(Cmp_hash* phash, char* filepath)
{
    FILE* file;
    unsigned char buffer[BUFFER_SIZE+1];
    int n = BUFFER_SIZE;
    uint32_t last_viewed[256] = {0};
    uint32_t max_gap[256] = {0};
    uint32_t index = 0;
    
    memset(phash, 0, sizeof(Cmp_hash));
    
    file = fopen(filepath, "rb");

    if(file == NULL)
        return FILE_NOT_FOUND;
    
    while(phash->size < MAX_U32 && n == BUFFER_SIZE)
    {
        n = fread(buffer, 1, BUFFER_SIZE, file);
        
        // We just look at the first 4 GB, beyond that the uint32_t are too small
        if(phash->size + n <= MAX_U32)
            _update_hash(phash, buffer, n, last_viewed, max_gap, index);
        
        phash->size += n;
        index += n;
    }

    fclose(file);

    return OK;
}

enum status_codes cmp_create_hash_from_url(Cmp_hash* phash, char* url)
{
    RequestsHandler* handler;
    unsigned char buffer[BUFFER_SIZE+1];
    int n = BUFFER_SIZE;
    uint32_t last_viewed[256] = {0};
    uint32_t max_gap[256] = {0};
    uint32_t index = 0;

    memset(phash, 0, sizeof(Cmp_hash));

    handler = req_get(url, "Connection: keep-alive\r\n");

    if(handler == NULL)
    {
        printf("get init error: %d\n", req_get_last_error());
        return UNKNOW_ERROR;
    }

    while(phash->size < MAX_U32 && n != 0)
    {
        errno = 0;
        n = req_read_output_body(handler, (char*)buffer, BUFFER_SIZE);
        
        if(n > 0)
        {
            //printf("%s\n", buffer);
            // We just look at the first 4 GB, beyond that the uint32_t are too small
            if(phash->size + n <= MAX_U32)
                _update_hash(phash, buffer, n, last_viewed, max_gap, index);
            
            phash->size += n;
            index += n;
        }
        else
        {
            printf("%d\n", phash->size);
            perror("");
        }
    }
    printf("hash size: %d\n", phash->size);
    printf("closing\n");
    req_close_connection(&handler);
    printf("closed\n");

    return OK;
}

double cmp_two_hashes(Cmp_hash* phash1, Cmp_hash* phash2)
{
    double total = 0.0;
    double division = 50.0;
    double temp;
    
    // coef 50
    total += 50.0 * corresponding_rate(phash1->size, phash2->size);
    
    for(int i=0; i < 256; i++)
    {
        // coef 3
        if((temp = corresponding_rate(phash1->data_occ[i], phash2->data_occ[i])) != -1)
        {
            total += 3.0 * temp;
            division += 3.0;
        }
    }
    
    for(int i=0; i < 256; i++)
    {
        // coef 1
        if((temp = corresponding_rate(phash1->data_gap[i], phash2->data_gap[i])) != -1)
        {
            total += 1.0 * temp;
            division += 1.0;
        }
    }
    
    return total / division;
}

double certainty(double corresponding_value)
{
    return exp(CORRECTION_VALUE*log(corresponding_value));
}

char check_hash_integrity(Cmp_hash* phash)
{
    uint32_t size = 0;

    for(int i = 0; i < 256; i++)
    {
        size += phash->data_occ[i];
    }

    return size == phash->size;
}