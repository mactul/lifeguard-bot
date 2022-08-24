#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmp_hash.h"

#define MAX_U32 4294967295


void _update_hash(Cmp_hash* phash, unsigned char* buffer, uint32_t* last_viewed, uint32_t* max_gap, uint32_t index)
{
    /*
    Internal function, used to update the hash with the data of the buffer
    */
    uint32_t i = 0;  // i is uint32_t to avoid implicit conversions
    while(buffer[i] != '\0')
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

uint32_t max_u32(uint32_t a, uint32_t b)
{
    if(a > b)
        return a;
    return b;
}

double min_on_max(double a, double b)
{
    if(a == b)
    {
        return 1.0; // useful for 0/0
    }
    if(a < b)
    {
        return a / b;
    }
    return b / a;
}

enum status_codes cmp_create_hash(Cmp_hash* phash, char* filepath)
{
    FILE* file;
    unsigned char buffer[BUFFER_SIZE+1];
    int n = BUFFER_SIZE;
    uint32_t last_viewed[256] = {};
    uint32_t max_gap[256] = {};
    uint32_t index = 0;
    
    memset(phash, 0, sizeof(Cmp_hash));
    
    file = fopen(filepath, "r");

    if(file == NULL)
        return FILE_NOT_FOUND;
    
    while(phash->size < MAX_U32 && n == BUFFER_SIZE)
    {
        n = fread(buffer, 1, BUFFER_SIZE, file);
        buffer[n] = '\0';
        
        // We just look at the first 4 GB, beyond that the uint32_t are too small
        if(phash->size + n <= MAX_U32)
            _update_hash(phash, buffer, last_viewed, max_gap, index);
        
        phash->size += n;
        index += n;
    }

    fclose(file);

    return OK;
}

double cmp_two_hashes(Cmp_hash* phash1, Cmp_hash* phash2)
{
    double total = 0.0;
    uint32_t max_gap = max_u32(phash1->size, phash2->size);
    
    // coef 50
    total += 50.0 * min_on_max(phash1->size, phash2->size);
    
    for(int i=0; i < 256; i++)
    {
        // coef 3
        total += 3.0 * min_on_max(phash1->data_occ[i], phash2->data_occ[i]);
    }
    
    for(int i=0; i < 256; i++)
    {
        // coef 1
        total += 1.0 - (double) abs(phash1->data_gap[i] - phash2->data_gap[i]) / (double) max_gap;
    }
    
    return total / (50.0 + 3.0 * 256.0 + 256.0);
}

double certainty(double corresponding_value)
{
    return exp(CORRECTION_VALUE*log(corresponding_value));
}