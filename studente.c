#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>



int main (int argc, char **argv) {
    int sockfd;
    int request, req;
    struct sockaddr_in servaddr;
    int c;

    //controllo indirizzo ip

    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <indirizzoIP>\n", argv[0]);
        exit(1);
    }
    //break per iniziare nuova connessione
    connessione:

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }


    //specifico la struttura dell'indirizzo del server

    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Errore inet_pton per %s\n", argv[1]);
        exit(1);
    }
    servaddr.sin_port = htons(1026);


     // Connessione al server.
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        printf("Connessione fallita!\n");
        exit(1);
    }
    else {
        printf("Connessione stabilita!\n");
    }

    //itero le operazioni che lo studente può fare

    while (1) {
        printf("\nInserire il numero relativo all'operazione che si vuole effettuare:\n");
        printf("1 - Visualizza appelli disponibili\n");
        printf("2 - Prenota un appello\n");
        printf("3 - Exit\n");
        printf("Scelta: ");
        scanf("%d", &request);
        printf("\n");

        //pulizia buffer input
        while ((c = getchar()) != '\n' && c != EOF);

        //invio scelta effettuata segreteria
        if (write(sockfd, &request, sizeof(request)) < 0) {
            printf("Connessione con la segreteria persa, ritento la connessione...\n");
            close(sockfd);
            goto connessione;

        }

        //lo studente può scegliere se visualizzare tutti gli appelli o solo quelli di un corso specifico
        if (request == 1) {
            char name[255] = {0};
            char date[12] = {0};
            int num_rows = 0;

            printf("Vuoi visualizzare tutti gli appelli o solo quelli relativi ad un determinato esame?");
            printf("\n1 - Tutti gli appelli");
            printf("\n2 - Esame di corso specifico");
            printf("\nScelta: ");
            scanf("%d", &req);


            //invio scelta alla segreteria

            if (write(sockfd, &req, sizeof(req)) < 0) {
                printf("Connessione con la segreteria persa, ritento la connessione...\n");
                close(sockfd);
                goto connessione;  //ritornare alla connessione

            }

            //pulisco il buffer

            while ((c = getchar()) != '\n' && c != EOF);

            //se la scelta è due bisogna mandare anche il nome del corso
            if (req == 2) {
                char exam[255] = {0};
                printf("\nInserisci nome corso: ");
                fgets(exam, sizeof(exam), stdin);
                exam[strlen(exam) - 1] = 0;

                if (write(sockfd, exam, strlen(exam)) < 0) {
                    printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                    close(sockfd);
                    goto connessione;

                }
            }

            //ricevuta numero appelli disponibili

            if (read(sockfd, &num_rows, sizeof(num_rows)) < 0) {
                printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                close(sockfd);
                goto connessione;
            }


            if (num_rows == 0) {
                printf("\nNon esistono appelli disponibili!\n");
            } else {
                printf("\nAppelli disponibili:\n");
                for (int i = 0; i < num_rows; i++) {

                    if (read(sockfd, name, sizeof(name)) < 0) {
                        printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                        close(sockfd);
                        goto connessione;

                    }

                    if (read(sockfd, date, sizeof(date)) < 0) {
                        printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                        close(sockfd);
                        goto connessione;

                    }

                    printf("%s\t%s\n", name, date);
                }
            }
        }

        //digita il nome e la data dell'appello specifico per inviare la richiesta di prenotazione

        else if (request == 2) {
            char exam_name[255], exam_date[255];

            printf("Digita il nome dell'appello al quale vuoi prenotarti: ");
            fgets(exam_name,sizeof(exam_name),stdin);
            exam_name[strcspn(exam_name, "\n")] = 0;

            if (write(sockfd, exam_name, sizeof(exam_name)) < 0) {
                printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                close(sockfd);
                goto connessione;
            }

            printf("Digita la data dell'appello al quale vuoi prenotarti: ");
            fflush(stdin); // Pulisce il buffer di input
            fgets(exam_date, sizeof(exam_date), stdin);
            exam_date[strcspn(exam_date, "\n")] = 0; // Rimuove il carattere di nuova riga

            if (write(sockfd, exam_date, sizeof(exam_date)) < 0) {
                printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                close(sockfd);
                goto connessione;
            }

            char res[255] = {0};

            //lo studente riceve l'esito dell'operazione con il numero progressivo

            if (read(sockfd, res, sizeof(res)) < 0) {
                printf("\nConnessione con la segreteria persa, ritento la connessione...\n");
                close(sockfd);
                goto connessione;

            }

            printf("\nEsito operazione: %s\n", res);

            if (strcmp(res, "Inserimento della nuova prenotazione completato con successo!") == 0) {
                int count;
                if (read(sockfd, &count, sizeof(count)) < 0) {
                    printf("Connessione con la segreteria persa, ritento la connessione...\n");
                    close(sockfd);
                    goto connessione;
                }

                printf("Numero prenotazione: %d", count);
                printf("\n");
            }
        }


        else if (request == 3) {
            printf("Programma in chiusura!\n");
            close(sockfd);
            exit(1);
        }
    }

}