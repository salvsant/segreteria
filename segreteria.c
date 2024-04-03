//
// Created by salva on 03/04/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>

typedef struct {
    int connfd;
} Client_stud;

int main(int argc, char **argv) {
    int sockfd, listenfd;
    int behaviour;
    int logical = 0;
    struct sockaddr_in servaddr, secaddr;
    MYSQL *conn;
    Client_stud client_sockets[4096];
}