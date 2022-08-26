#include "database.h"

int main()
{

    printf("%d\n", check_db_integrity());
    add_db_from_folder("malwares/files/", 1);
    printf("%d\n", check_db_integrity());

    return 0;
}