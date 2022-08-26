#include "database.h"
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

char get_next_malware_hash(Cmp_hash* phash, FILE* fptr)
{
    int n = fread(phash, 1, sizeof(Cmp_hash), fptr);
    if(n != sizeof(Cmp_hash) && n != 0)
        return -1;
    return !feof(fptr);
}

double best_malware_correspondance(Cmp_hash* phash)
{
    Cmp_hash current_hash;
    FILE* fptr;
    double correspondance = 0.0;
    double max_corr = 0.0;
    int counter = 0;

    if ((fptr = fopen(DB_FILE_NAME,"rb")) == NULL)
    {
        printf("Error! opening file");
        return 0.0;
    }
    while(get_next_malware_hash(&current_hash, fptr) && max_corr != 1.0)
    {
        correspondance = cmp_two_hashes(phash, &current_hash);
        if(correspondance > max_corr)
        {
            max_corr = correspondance;
        }
        counter++;
    }
    fclose(fptr);

    return certainty(max_corr);
}


int64_t check_db_integrity()
{
    FILE* fptr;
    Cmp_hash hash;
    char integrity = 1;
    char result;

    if(access(DB_FILE_NAME, F_OK) != 0)
    {
        // file not exists, so DB has integrity
        return DB_INTEGRITY;
    }

    if ((fptr = fopen(DB_FILE_NAME,"rb")) == NULL)
    {
        printf("Error! opening file");
        return 0;
    }

    while((result = get_next_malware_hash(&hash, fptr)) && result != -1 && (integrity = check_hash_integrity(&hash)))
    {
        ;
    }
    if(integrity && result != -1)
    {
        fclose(fptr);
        return DB_INTEGRITY;
    }
    else
    {
        int64_t size = 2052 * (int64_t)(ftell(fptr)/2052) - sizeof(Cmp_hash);
        if(size < 0)
            size = 0;
        
        fclose(fptr);
        return size;
    }
}

void add_db_from_folder(char* folder_path, char check_if_exists)
{
    DIR *d;
    struct dirent *dir;
    FILE *fptr;
    int64_t integrity;

    if((integrity = check_db_integrity()) != DB_INTEGRITY)
    {
        printf("%d\n", integrity);
        truncate(DB_FILE_NAME, integrity);
    }

    if ((fptr = fopen(DB_FILE_NAME,"ab")) == NULL)
    {
        printf("Error! opening file");
        return;
    }
    
    d = opendir(folder_path);
    if (d)
    {
        int counter = 0;
        while ((dir = readdir(d)) != NULL)
        {
            char path[1024];

            strcpy(path, folder_path);
            strcat(path, dir->d_name);
            if(is_regular_file(path))
            {
                Cmp_hash hash;
                if(cmp_create_hash(&hash, path) == OK && hash.size != 0)
                {
                    if(!check_if_exists || best_malware_correspondance(&hash) != 1.0)
                        fwrite(&hash, sizeof(hash), 1, fptr);
                    remove(path); // delete the file
                    counter++;
                    printf("%d\n", counter);
                }
            }
        }
        closedir(d);
        fclose(fptr);
    }
}
