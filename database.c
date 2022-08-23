#include "cmp_hash.h"
#include <dirent.h>
#include <stdio.h>
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

char get_next_malware_hash(Cmp_hash* phash)
{
    static int pos = 0;
    static FILE *fptr;
    if(pos == 0)
    {
        if ((fptr = fopen("db.bin","rb")) == NULL)
        {
            printf("Error! opening file");
            return;
        }
    }
    fread(phash, sizeof(Cmp_hash), 1, fptr);
    
    if(feof(fptr))
    {
        fclose(fptr);
        pos = 0;
        return 0;
    }
    return 1;
}


void create_db_from_folder(char* folder_path)
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
    create_db_from_folder("malwares/files/");
}