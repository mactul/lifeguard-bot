#include "database.h"
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mysql.h>
#include "db_credentials.h"


void finish_with_error(MYSQL* con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  //exit(1);
}


int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

char get_next_malware_hash(Cmp_hash* phash, MYSQL_RES *result)
{
    MYSQL_ROW row;
    row = mysql_fetch_row(result);
    if(row == NULL)
    {
        return 0;
    }
    memcpy(phash, row[0], sizeof(Cmp_hash));
    return 1;
}


double best_malware_correspondance(Cmp_hash* phash)
{
    Cmp_hash current_hash;
    double correspondance = 0.0;
    double max_corr = 0.0;
    char query[256];

    MYSQL* con = mysql_init(NULL);

    if (mysql_real_connect(con, DB_HOST, DB_LOGIN, DB_PASSWORD,
          DB_NAME, 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
        return 0.0;
    }

    sprintf(query, "SELECT hash FROM virus WHERE size >= %llu AND size <= %llu", (uint64_t) (0.9 * (double)phash->size), (uint64_t) (1.1 * (double)phash->size));

    if (mysql_query(con, query))
    {
        finish_with_error(con);
        return 0.0;
    }
    
    MYSQL_RES *result = mysql_store_result(con);
    
    while(get_next_malware_hash(&current_hash, result) && max_corr != 1.0)
    {
        correspondance = cmp_two_hashes(phash, &current_hash);
        printf("%f\n", correspondance);
        if(correspondance > max_corr)
        {
            max_corr = correspondance;
        }
    }
    mysql_free_result(result);
    mysql_close(con);

    return certainty(max_corr);
}

char read_next_hash_in_file(Cmp_hash* phash, FILE* fptr)
{
    int n = fread(phash, 1, sizeof(Cmp_hash), fptr);
    if(n != sizeof(Cmp_hash) && n != 0)
        return -1;
    return !feof(fptr);
}

void convert_old_db(void)
{
    Cmp_hash current_hash;
    FILE* fptr;
    MYSQL *con = mysql_init(NULL);

    if (mysql_real_connect(con, DB_HOST, DB_LOGIN, DB_PASSWORD,
          DB_NAME, 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
        return;
    }

    if ((fptr = fopen(DB_FILE_NAME,"rb")) == NULL)
    {
        printf("Error! opening file");
        return;
    }
    while(read_next_hash_in_file(&current_hash, fptr))
    {
        if(check_hash_integrity(&current_hash))
        {
            char chunk[2*sizeof(Cmp_hash)+1];
            mysql_real_escape_string(con, chunk, (char*)&current_hash, sizeof(Cmp_hash));
            char *st = "INSERT INTO virus(size, hash) VALUES('%d', '%s')";
            size_t st_len = strlen(st);

            char query[st_len + 2*sizeof(Cmp_hash)+1];
            int len = snprintf(query, st_len + 2*sizeof(Cmp_hash)+1, st, current_hash.size, chunk);

            if (mysql_real_query(con, query, len))
            {
                fprintf(stderr, "%s\n", mysql_error(con));
            }
        }
    }
    mysql_close(con);
    fclose(fptr);
}


void add_db_from_folder(char* folder_path)
{
    DIR *d;
    struct dirent *dir;
    MYSQL *con = mysql_init(NULL);

    if (mysql_real_connect(con, DB_HOST, DB_LOGIN, DB_PASSWORD,
          DB_NAME, 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
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
                    if(check_hash_integrity(&hash))
                    {
                        char chunk[2*sizeof(Cmp_hash)+1];
                        mysql_real_escape_string(con, chunk, (char*)&hash, sizeof(Cmp_hash));
                        char *st = "INSERT INTO virus(size, hash) VALUES('%d', '%s')";
                        size_t st_len = strlen(st);

                        char query[st_len + 2*sizeof(Cmp_hash)+1];
                        int len = snprintf(query, st_len + 2*sizeof(Cmp_hash)+1, st, hash.size, chunk);

                        if (mysql_real_query(con, query, len))
                        {
                            fprintf(stderr, "%s\n", mysql_error(con));
                        }
                    }
                    remove(path); // delete the file
                    counter++;
                    printf("%d\n", counter);
                }
            }
        }
        closedir(d);
        mysql_close(con);
    }
}
