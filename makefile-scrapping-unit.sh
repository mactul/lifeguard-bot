gcc -o scrapping_unit scrapping_unit.c easy_tcp_tls.c requests.c utils.c parser_tree.c -I/openssl/* -I/mysql/ -lcrypto -lssl -lmysqlclient -lpthread -lm -DLinux -Wall

