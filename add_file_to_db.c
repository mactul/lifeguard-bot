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

    cmp_create_hash(&hash, argv[1]);

    add_hash_to_db(&hash);

    return 0;
}