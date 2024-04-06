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

    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <indirizzoIP>\n", argv[0]);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }


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

    while (1) {
        printf("\nInserire il numero relativo all'operazione che si vuole effettuare:\n");
        printf("1 - Visualizza appelli disponibili\n");
        printf("2 - Prenota un appello\n");
        printf("3 - Exit\n");
        printf("Scelta: ");
        scanf("%d", &request);
        printf("\n");


        while ((c = getchar()) != '\n' && c != EOF);


        if (write(sockfd, &request, sizeof(request)) < 0) {
            printf("Connessione con la segreteria persa, ritento la connessione...\n");
            close(sockfd);
            goto connessione;

        }


        if (request == 1) {
            char name[255] = {0};
            char date[12] = {0};
            int num_rows = 0;

            printf("Vuoi visualizzare tutti gli appelli o solo quelli relativi ad un determinato esame?");
            printf("\n1 - Tutti gli appelli");
            printf("\n2 - Esame di corso specifico");
            printf("\nScelta: ");
            scanf("%d", &req);


            if (write(sockfd, &req, sizeof(req)) < 0) {
                printf("Connessione con la segreteria persa, ritento la connessione...\n");
                close(sockfd);
                goto connessione;

            }


            while ((c = getchar()) != '\n' && c != EOF);

          +
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
    }

}