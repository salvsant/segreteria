#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <mysql/mysql.h>
#include <pthread.h>

void addExamSession(int);
void addBooking(int);
void addExam(int);
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

    fd_set read_set;
    int max_fd;
    max_fd = listenfd;


/*
 * ciclo while "infinito" per instaurare nuove connessioni
 */
    while(1) {

        FD_ZERO(&read_set);
        FD_SET(listenfd, &read_set);


        if (connfd > -1) {
            FD_SET(connfd, &read_set);
        }


        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("Errore nell'operazione di select!");
        }


        if (FD_ISSET(listenfd, &read_set)) {

            if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                perror("Errore nell'operazione di accept!");
            }


            if (connfd > max_fd) {
                max_fd = connfd;
            }
        }
        if (FD_ISSET(connfd, &read_set)) {

            if (read(connfd, &request, sizeof(request)) > 0) {

                if (request == 1) {
                    addExamSession(connfd);
                } else if (request == 2) {
                    addBooking(connfd);
                } else if (request == 3){
                    addExam(connfd);
                }
            }
        }
    }






    printf("Hello, World!\n");
    printf("test commit");
    return 0;
}
