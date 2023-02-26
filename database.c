#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mysql/mysql.h>
#include "cmp_hash.h"
#include "database.h"
#include "db_credentials.h"


void finish_with_error(MYSQL* con)
{
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
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

    sprintf(query, "SELECT hash FROM virus WHERE size >= %llu AND size <= %llu", (unsigned long long) (0.95 * (double)phash->size), (unsigned long long) (1.05 * (double)phash->size));

    if (mysql_query(con, query))
    {
        finish_with_error(con);
        return 0.0;
    }
    
    MYSQL_RES *result = mysql_store_result(con);
    
    while(get_next_malware_hash(&current_hash, result) && max_corr != 1.0)
    {
        correspondance = cmp_two_hashes(phash, &current_hash);
        if(correspondance > max_corr)
        {
            max_corr = correspondance;
        }
    }
    mysql_free_result(result);
    mysql_close(con);

    return certainty(max_corr);
}


void add_hash_to_db(Cmp_hash* phash)
{
    MYSQL *con = mysql_init(NULL);

    if (mysql_real_connect(con, DB_HOST, DB_LOGIN, DB_PASSWORD,
          DB_NAME, 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
        return;
    }

    if(phash->size != 0 && check_hash_integrity(phash))
    {
        char chunk[2*sizeof(Cmp_hash)+1];
        mysql_real_escape_string(con, chunk, (char*)phash, sizeof(Cmp_hash));
        char *st = "INSERT INTO virus(size, hash) VALUES('%d', '%s')";
        size_t st_len = strlen(st);

        char query[st_len + 2*sizeof(Cmp_hash)+1];
        int len = snprintf(query, st_len + 2*sizeof(Cmp_hash)+1, st, phash->size, chunk);

        if (mysql_real_query(con, query, len))
        {
            fprintf(stderr, "%s\n", mysql_error(con));
        }
    }
}
