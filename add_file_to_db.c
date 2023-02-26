#include <stdint.h>
#include <stdio.h>
#include "cmp_hash.h"
#include "database.h"

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Error, this program takes a single argument\n");
        return 1;
    }

    Cmp_hash hash;

    if(cmp_create_hash(&hash, argv[1]) == OK)
    {
        add_hash_to_db(&hash);
    }
    else
    {
        printf("File Not Found\n");
    }

    return 0;
}