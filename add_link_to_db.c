#include <stdint.h>
#include <stdio.h>
#include "requests.h"
#include "cmp_hash.h"
#include "database.h"

int main(int argc, char* argv[])
{
    req_init();

    if(argc != 2)
    {
        printf("Error, this program takes a single argument\n");
        return 1;
    }

    Cmp_hash hash;

    printf("%s\n", argv[1]);

    cmp_create_hash_from_url(&hash, argv[1]);

    printf("%d\n", hash.size);

    add_hash_to_db(&hash);

    req_cleanup();

    return 0;
}