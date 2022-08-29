#include "database.h"

int main()
{
    Cmp_hash hash;

    cmp_create_hash(&hash, "virus_checker");

    printf("Malware variant recognized with %f%% certainty\n", 100*best_malware_correspondance(&hash));
    
    return 0;
}