#include <stdint.h>

#define BUFFER_SIZE 2048
#define CORRECTION_VALUE 6.57881348  // log(0.5)/log(0.9)
                                     // because 0.5 = exp(x*log(0.9)) <=> x = log(0.5)/log(0.9)
                                     // Like that, 90% corresponding <=> 50% certainty

typedef struct cmp_hash {
    uint32_t size;
    uint32_t data_occ[256]; // nb occ char
    uint32_t data_gap[256]; // index max ecart
} Cmp_hash;

enum status_codes {
    OK,
    UNKNOW_ERROR,
    FILE_NOT_FOUND
};

enum status_codes cmp_create_hash(Cmp_hash* phash, char* filepath);
enum status_codes cmp_create_hash_from_url(Cmp_hash* phash, char* url);
double cmp_two_hashes(Cmp_hash* phash1, Cmp_hash* phash2);
double certainty(double corresponding_value);
char check_hash_integrity(Cmp_hash* phash);