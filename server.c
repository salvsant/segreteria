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

    //errore creazione socket
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

    //errore operazione bind
    if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof (servaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        exit(1);
    }

    //errore operazione listen
    if ((listen(listenfd, 1024)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }

    printf("Hello, World!\n");
    printf("test commit");
    return 0;
}
