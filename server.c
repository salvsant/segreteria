#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>
#include <pthread.h>

typedef struct {
    int connfd;
} Client_stud;

int main() {
    int sockfd, listenfd;
    int dim = 0;
    struct sockaddr_in servaddr, secaddr;
    MYSQL *conn;
    Client_stud client_sockets[4096];


    printf("Hello, World!\n");
    printf("test commit");
    return 0;
}
