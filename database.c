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

void create_db_from_folder(char* folder_path)
{
    DIR *d;
    struct dirent *dir;
    FILE *fptr;

    if ((fptr = fopen("db.bin","ab")) == NULL)
    {
        printf("Error! opening file");
        exit(1);
    }
    
    d = opendir(folder_path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if(is_regular_file(dir->d_name))
            {
                Cmp_hash hash;

                if(cmp_create_hash(&hash, dir->d_name) == OK)
                {
                    printf("%s\n", dir->d_name);
                    printf("%d\n", hash.size);
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
    create_db_from_folder(".");
}