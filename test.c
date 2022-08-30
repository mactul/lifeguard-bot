#include "database.h"

int main()
{
    Cmp_hash hash;

    cmp_create_hash(&hash, "test.png");

    printf("Malware variant recognized with %f%% certainty\n", 100*best_malware_correspondance(&hash));


    cmp_create_hash_from_url(&hash, "https://cdn.discordapp.com/attachments/699999438269186058/1002993123967897770/stats.png");

    printf("Malware variant recognized with %f%% certainty\n", 100*best_malware_correspondance(&hash));
    
    return 0;
}