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
    fread(phash, sizeof(Cmp_hash), 1, fptr);
    
    return !feof(fptr);
}

double best_malware_correspondance(Cmp_hash* phash)
{
    Cmp_hash current_hash;
    FILE* fptr;
    double correspondance = 0.0;
    double max_corr = 0.0;
    int counter = 0;

    if ((fptr = fopen("db.bin","rb")) == NULL)
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


void add_db_from_folder(char* folder_path, char check_if_exists)
{
    DIR *d;
    struct dirent *dir;
    FILE *fptr;

    if ((fptr = fopen("db.bin","ab")) == NULL)
    {
        printf("Error! opening file");
        return;
    }
    
    d = opendir(folder_path);
    if (d)
    {
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
                }
            }
        }
        closedir(d);
        fclose(fptr);
    }
}
