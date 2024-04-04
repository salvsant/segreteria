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
    struct timeval timeout;
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

            for (int i=0; i < dim; i++) {


                if (FD_ISSET(client_sockets[i].connfd, &read_set) && client_sockets[i].connfd != -1) {



                    read(client_sockets[i].connfd, &behaviour, sizeof(behaviour));


                    if (behaviour == 1) {

                        int req;
                        read(client_sockets[i].connfd, &req, sizeof(req));

                        char query[500];


                        if (req == 1) {
                            snprintf(query, sizeof(query), "SELECT exam_session_name, DATE_FORMAT(session_date, '%%Y-%%m-%%d') FROM exam_sessions");

                            if (mysql_query(conn, query) != 0) {
                                fprintf(stderr, "mysql_query() fallita\n");

                            }

                        }

                    }
                    else if (req == 2) {
                        char exam[255] = {0};
                        /**
                         * La segreteria riceve il nome dell'esame di cui lo studente vuole visualizzare gli appelli
                         * disponibili.
                         */
                        read(client_sockets[i].connfd, exam, sizeof(exam));

                        snprintf(query, sizeof(query), "SELECT exam_session_name, DATE_FORMAT(session_date, '%%Y-%%m-%%d') "
                                                       "FROM exam_sessions "
                                                       "WHERE exam_session_name IN  (SELECT exam_name FROM exams WHERE nome_corso = '%s')", exam);


                        if (mysql_query(conn, query) != 0) {
                            fprintf(stderr, "mysql_query() fallita\n");
                        }
                    }

                    MYSQL_RES *res2 = mysql_store_result(conn);
                    if (res2 == NULL) {
                        fprintf(stderr, "mysql_store_result() fallita\n");
                    }
                    else {
                        /**
                         * Inviamo allo studente il numero di righe risultanti dalla query, in modo che lo studente
                         * possa visualizzare correttamente tutti i campi degli appelli disponibili.
                         */
                        unsigned int num_rows = mysql_num_rows(res2);
                        write(client_sockets[i].connfd, &num_rows, sizeof(num_rows));

                        char exam_name[255];
                        char exam_date[12];

                        /**
                         * Inviamo allo studente tutti i campi di tutte le righe risultanti dalla query.
                         */
                        MYSQL_ROW row;
                        while ((row = mysql_fetch_row(res2))) {
                            sscanf(row[0], "%[^\n]", exam_name);
                            sscanf(row[1], "%s", exam_date);
                            write(client_sockets[i].connfd, exam_name, sizeof(exam_name));
                            write(client_sockets[i].connfd, exam_date, sizeof(exam_date));
                        }
                    }

                    mysql_free_result(res2);
                }

                else if (behaviour == 2) {
                    char exam_name[255], exam_date[255];
                    char res[255] = {0};


                    write(sockfd, &behaviour, sizeof(behaviour));
                    perror("behaviour");



                    read(client_sockets[i].connfd, exam_name, sizeof(exam_name));
                    perror("read1");

                    read(client_sockets[i].connfd, exam_date, sizeof(exam_date));
                    perror("read2");



                    write(sockfd, exam_name, sizeof(exam_name));
                    perror("write1");

                    write(sockfd, exam_date, sizeof(exam_date));
                    perror("write2");



                    read(sockfd, res, sizeof(res));
                    perror("readserver");



                    write(client_sockets[i].connfd, res, sizeof(res));
                    perror("write");



                    if (strcmp(res, "Inserimento della nuova prenotazione completato con successo!") == 0) {
                        int count;
                        read(sockfd, &count, sizeof(count));
                        write(client_sockets[i].connfd, &count, sizeof(count));
                    }
                }
            }
        }

        if (FD_ISSET(sockfd, &write_set)) {

            FD_ZERO(&read_set);
            FD_SET(STDIN_FILENO, &read_set);

                if(test == 0) {
                printf("Vuoi gestire le richieste degli studenti o inserire un nuovo appello?\n");
                printf("Digitare qualsiasi numero - Gestire le richieste degli studenti\n");
                printf("2 - Inserire un nuovo appello\n");
                printf("3- Inserisci un esame\n");
                printf("Scelta: ");
                test = 1;}


            timeout.tv_sec = 1; // 1 secondo timeout
            timeout.tv_usec = 0;


            if(test == 1){
                int ready = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
                if (ready == -1) {
                    perror("select");
                    return 1;
                } else if (ready == 0) {

                    continue;
                }




                scanf("%d", &logical);




                printf("\n");

                }
            }









