#include "cmp_hash.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    char hashes_left = 1;
    double correspondance = 0.0;
    double max_corr = 0.0;

    if ((fptr = fopen("db.bin","rb")) == NULL)
    {
        printf("Error! opening file");
        return 0.0;
    }
    while(hashes_left && correspondance != 1.0)
    {
        hashes_left = get_next_malware_hash(&current_hash, fptr);
        correspondance = cmp_two_hashes(phash, &current_hash);
        if(correspondance > max_corr)
        {
            max_corr = correspondance;
        }
    }
    fclose(fptr);

    return certainty(max_corr);
}


void create_db_from_folder(char* folder_path)
{
    DIR *d;
    struct dirent *dir;
    FILE *fptr;

    if ((fptr = fopen("db.bin","wb")) == NULL)
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
                if(cmp_create_hash(&hash, path) == OK)
                {
                    fwrite(&hash, sizeof(hash), 1, fptr);
                }
            }
        }
        closedir(d);
        fclose(fptr);
    }
}

int main()
{
    Cmp_hash hash;

    //create_db_from_folder("malwares/files/");


    cmp_create_hash(&hash, "central.c");
    printf("%f\n", best_malware_correspondance(&hash));
}