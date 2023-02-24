gcc -o central central.c easy_tcp_tls.c database.c queue.c cmp_hash.c requests.c utils.c parser_tree.c -I/openssl/* -I/mysql/ -lcrypto -lssl -lmysqlclient -lpthread -lm -DLinux -Wall

