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
    int logical = 0;
    int dim = 0;
    int test = 0;
    struct sockaddr_in servaddr, secaddr;
    struct timeval timeout;
    char name[255] = {0};
    char date[12] = {0};
    char result[255] = {0};
    char corso[255];
    MYSQL *conn;
    Client_stud client_sockets[4096];

    // SEGRETERIA CLIENT
    // Controllo inserimento ip
    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <indirizzoIP>\n", argv[0]);
        exit(1);
    }

    //Creazione socket
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

/** Specifico la struttura dell'indirizzo del server al quale ci si vuole connettere tramite i campi di una struct
     * di tipo sockaddr_in.*/
    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Errore inet_pton per %s\n", argv[1]);
        exit(1);
    }
    servaddr.sin_port = htons(1025);

    //connessione
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Errore nell'operazione di connect!");
        exit(1);
    }


    // SEGRETERIA SERVER

    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

    //specifico la struttura dell'indirizzo del server
    secaddr.sin_family = AF_INET;
    secaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    secaddr.sin_port = htons(1026);

/**
     * La system call bind permette di assegnare l'indirizzo memorizzato nel campo s_addr della struct sin_addr, che è
     * a sua volta un campo della struct sockaddr_in (secaddr nel nostro caso), al descrittore listenfd.
     */

    if ((bind(listenfd, (struct sockaddr *)&secaddr, sizeof (secaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        exit(1);
    }

    //Server in ascolto
    if ((listen(listenfd, 5)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }

    //inizializza database
    conn = mysql_init(NULL);

    if (conn == NULL) {

        fprintf(stderr, "mysql_init() fallita\n");
        exit(1);
    }

    //connessione al database mysql
    if (mysql_real_connect(conn, "95.252.224.133", "admin", "admin", "nuova_segreteria", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    /**
        * Ci serviamo di 3 "insiemi" di descrittori per strutturare la successiva select.
        * In particolare abbiamo read_set e write_set che mantengono l'insieme dei descrittori, rispettivamente in lettura
        * e scrittura, e master_set, che permette di reinizializzare read_set e write_set ad ogni iterazione.
        */

    fd_set read_set, write_set, master_set;
    int max_fd;

    //Inizializzo a 0 tutti i descrittori
    FD_ZERO(&master_set);

    FD_SET(sockfd, &master_set);
    max_fd = sockfd;

    FD_SET(listenfd, &master_set);
    max_fd = max(max_fd, listenfd);

    //while esterno per ottimizzare velocità
    while(1) {
        read_set = master_set;
        write_set = master_set;

        //while interno per soddisfare le richieste dei client
        while (1) {

            /**
            * La funzione select restituisce il numero di descrittori pronti, a partire dagli "insiemi" di descrittori
            * passati.
            */

            if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0) {
                perror("Errore nell'operazione di select!");
            }

            //controllo listenfd per instaurare nuove connessioni

            if (FD_ISSET(listenfd, &read_set)) {

                //system call accept per accettare nuove connessioni

                if ((client_sockets[dim].connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                    perror("Errore nell'operazione di accept!");
                }
                else {

                    //aggiunta descrittore legata nuova connessione

                    FD_SET(client_sockets[dim].connfd, &master_set);
                    max_fd = max(max_fd, client_sockets[dim].connfd);


                    dim++;

                }
            }
            else {
                break;
            }
        }

        /**
                * Itero in base al valore di dim in modo da soddisfare le richieste di tutti gli studenti connessi in modo
                * concorrente.
                */

        for (int i=0; i < dim; i++) {

            //controllo dei descrittori delle socket di connessione dei client

            if (FD_ISSET(client_sockets[i].connfd, &read_set) && client_sockets[i].connfd != -1) {


                //la segreteria riceve le scelte delle operazioni da fare
                read(client_sockets[i].connfd, &behaviour, sizeof(behaviour));

                //scelta 1- visualizza appelli disponibili
                if (behaviour == 1) {

                    int req;
                    read(client_sockets[i].connfd, &req, sizeof(req));

                    //stringa che contiene la query per effettuare l'operazione 1
                    char query[500];

                    //req = 1 tutti gli appelli
                    if (req == 1) {
                        snprintf(query, sizeof(query), "SELECT exam_session_name, DATE_FORMAT(session_date, '%%Y-%%m-%%d') FROM exam_sessions");

                        if (mysql_query(conn, query) != 0) {
                            fprintf(stderr, "mysql_query() fallita\n");

                        }

                    }

                    //req = 2 appelli di uno specifico corso
                    else if (req == 2) {
                        char exam[255] = {0};

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

                        //invio dei risultati allo studente

                        unsigned int num_rows = mysql_num_rows(res2);
                        write(client_sockets[i].connfd, &num_rows, sizeof(num_rows));

                        char exam_name[255];
                        char exam_date[12];

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


                //scelta due- studente vuole prenotare un appello

                else if (behaviour == 2) {
                    char exam_name[255], exam_date[255];
                    char res[255] = {0};

                    /**
                    * La prenotazione deve avvenire tramite il server universitario, quindi inoltriamo la richiesta
                    * dello studente al server universitario.
                    */

                    write(sockfd, &behaviour, sizeof(behaviour));
                    perror("behaviour");



                    read(client_sockets[i].connfd, exam_name, sizeof(exam_name));
                    perror("read1");

                    read(client_sockets[i].connfd, exam_date, sizeof(exam_date));
                    perror("read2");


                    //inoltro dati ricevuti da studente a server

                    write(sockfd, exam_name, sizeof(exam_name));
                    perror("write1");

                    write(sockfd, exam_date, sizeof(exam_date));
                    perror("write2");



                    //ricezione esito operazione dal server

                    read(sockfd, res, sizeof(res));
                    perror("readserver");


                    //esito inoltrato allo studente

                    write(client_sockets[i].connfd, res, sizeof(res));
                    perror("write");

                    //invio del numero progressivo allo studente relativo all'esame prenotato

                    if (strcmp(res, "Inserimento della nuova prenotazione completato con successo!") == 0) {
                        int count;
                        read(sockfd, &count, sizeof(count));
                        write(client_sockets[i].connfd, &count, sizeof(count));
                    }
                }
            }
        }


        /*controllo se la segreteria è pronta in scrittura
        legge gli eventi in console e se non succede nulla prima del timeout
         si resetta il loop
         */
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


            timeout.tv_sec = 1; // 1 secondi timeout
            timeout.tv_usec = 0;


            if(test == 1){
                int ready = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
                if (ready == -1) {
                    perror("select");
                    return 1;
                } else if (ready == 0) {
                    // Timeout, ritorni all'inizio del while
                    continue;
                }




                scanf("%d", &logical);




                printf("\n");

                //pulizia buffer input

                int c;
                while ((c = getchar()) != '\n' && c != EOF);


                test =  2;}

            /**
             * Nel caso in cui si voglia aggiungere un nuovo appello, si procede alla comunicazione con il server
             * universitario.
             */

            if (logical == 2) {

                if(test == 2){
                    int request = 1;

                    write(sockfd, &request, sizeof(request));

                    printf("Inserire il nome dell'esame di cui si vuole aggiungere un appello:\n");
                    test = 3;}

                if (test == 3){

                    int ready = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
                    if (ready == -1) {
                        perror("select");
                        return 1;
                    } else if (ready == 0) {
                        continue;
                    }


                    fgets(name, sizeof(name), stdin);
                    name[strlen(name) - 1] = 0;

                    printf("\n");

                    //invio nome esame di cui si vuole aggiungere un appello

                    write(sockfd, name, strlen(name));


                    printf("Inserire la data del nuovo appello (in formato YYYY-MM-DD):\n");

                    test = 4;}


                if(test == 4){
                    int ready = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
                    if (ready == -1) {
                        perror("select");
                        return 1;
                    } else if (ready == 0) {
                        continue;
                    }

                    fgets(date, sizeof(date), stdin);
                    date[strlen(date) - 1] = 0;

                    //invio data di appello

                    write(sockfd, date, strlen(date));

                    //ricezione esito operazione

                    read(sockfd, result, sizeof(result));

                    printf("\nEsito: %s\n\n", result);
                    test = 0;}
            }

            //la segreteria vuole aggiungere un esame

            if (logical == 3) {
                if(test == 2){
                    int request = 3;


                    write(sockfd, &request, sizeof(request));
                    printf("Inserire il nome dell'esame che si vuole aggiungere:\n");
                    test = 3;}


                if (test == 3){
                    int ready = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
                    if (ready == -1) {
                        perror("select");
                        return 1;
                    } else if (ready == 0) {
                        continue;
                    }
                    fgets(name, sizeof(name), stdin);
                    name[strlen(name) - 1] = 0;
                    printf("\n");

                    write(sockfd, name, strlen(name));

                    printf("Inserire il nome del corso :\n");
                    test = 4;}


                if(test == 4) {
                    int ready = select(STDIN_FILENO + 1, &read_set, NULL, NULL, &timeout);
                    if (ready == -1) {
                        perror("select");
                        return 1;
                    } else if (ready == 0) {
                        continue;
                    }

                    fgets(corso, sizeof(corso), stdin);
                    corso[strlen(corso) - 1] = 0;

                    write(sockfd, corso, strlen(corso));


                    read(sockfd, result, sizeof(result));

                    printf("\nEsito: %s\n\n", result);
                    test = 0;
                }




            }
        }
    }
}


