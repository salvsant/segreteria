#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>
#include <pthread.h>

MYSQL *connection();

int main() {
    int listenfd,connfd=-1;
    int request;
    struct sockaddr_in servaddr;



    printf("Hello, World!\n");
    printf("test commit");
    return 0;
}
