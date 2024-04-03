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

#define max(x, y) ({typeof (x) x_ = (x); typeof (y) y_ = (y); \
x_ > y_ ? x_ : y_;})

typedef struct {
    int connfd;
} Client_stud;

int main(int argc, char **argv) {
    int sockfd, listenfd;
    int behaviour;
    int dim = 0;
    int test = 0;
    int logical = 0;
    char name[255] = {0};
    char date[12] = {0};
    char result[255] = {0};
    char corso[255];
    struct sockaddr_in servaddr, secaddr;
    MYSQL *conn;
    Client_stud client_sockets[4096];


    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <indirizzoIP>\n", argv[0]);
        exit(1);
    }


    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }


    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Errore inet_pton per %s\n", argv[1]);
        exit(1);
    }
    servaddr.sin_port = htons(1025);


    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Errore nell'operazione di connect!");
        exit(1);
    }

    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }


    secaddr.sin_family = AF_INET;
    secaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    secaddr.sin_port = htons(1026);


    if ((bind(listenfd, (struct sockaddr *)&secaddr, sizeof (secaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        exit(1);
    }


    if ((listen(listenfd, 5)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }


    conn = mysql_init(NULL);

    if (conn == NULL) {

        fprintf(stderr, "mysql_init() fallita\n");
        exit(1);
    }

    if (mysql_real_connect(conn, "87.11.19.92", "admin", "admin", "nuova_segreteria", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    fd_set read_set, write_set, master_set;
    int max_fd;


    FD_ZERO(&master_set);

    FD_SET(sockfd, &master_set);
    max_fd = sockfd;

    FD_SET(listenfd, &master_set);
    max_fd = max(max_fd, listenfd);

    while(1)
    {
        read_set = master_set;
        write_set = master_set;

        while (1) {
            /**
             * La funzione select restituisce il numero di descrittori pronti, a partire dagli "insiemi" di descrittori
             * passati.
             */
            if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0) {
                perror("Errore nell'operazione di select!");
            }

            /**
             * Si controlla se il descrittore listenfd, ossia quello che monitora le nuove richieste di connessioni,
             * sia pronto, il che è vero quando appunto ci sono nuove connessioni in attesa.
             */
            if (FD_ISSET(listenfd, &read_set)) {
                /**
                 * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
                 */
                if ((client_sockets[dim].connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                    perror("Errore nell'operazione di accept!");
                }
                else {
                    /**
                     * Si aggiunge il descrittore legato alla nuova connessione da uno studente all'interno dell'array di
                     * descrittori master_set e si ricalcola il numero di posizioni da controllare nella select.
                     */
                    FD_SET(client_sockets[dim].connfd, &master_set);
                    max_fd = max(max_fd, client_sockets[dim].connfd);


                    dim++;

                }
            }
            else {
                break;
            }


}
    }
}



