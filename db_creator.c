#include "database.h"

int main(int argc, char* argv[])
{
    char check_existance = 1;
    if(argc == 2 && argv[1][0] == '1')
    {
        check_existance = 0;
    }
    add_db_from_folder("malwares/files/", check_existance);
    printf("%d\n", check_db_integrity());

    return 0;
}