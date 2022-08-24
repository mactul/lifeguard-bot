#include "cmp_hash.h"
#include <stdio.h>
#include <stdlib.h>

char get_next_malware_hash(Cmp_hash* phash, FILE* fptr);
double best_malware_correspondance(Cmp_hash* phash);
void create_db_from_folder(char* folder_path);