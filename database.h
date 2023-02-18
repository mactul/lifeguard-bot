#define _FILE_OFFSET_BITS 64  // this is because files can have more than 2GB

#define DB_FILE_NAME "db.bin"
#define SORTED_DB_FILE_NAME "sorted_db.bin"

#define DB_INTEGRITY -1

double best_malware_correspondance(Cmp_hash* phash);
void add_hash_to_db(Cmp_hash* phash);
void convert_old_db(void);