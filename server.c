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
    int listenfd, connfd = -1;
    int request;
    struct sockaddr_in servaddr;

    //errore creazione socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

    //errore operazione bind
    if ((bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
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
    while (1) {

        FD_ZERO(&read_set);
        FD_SET(listenfd, &read_set);


        if (connfd > -1) {
            FD_SET(connfd, &read_set);
        }


        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("Errore nell'operazione di select!");
        }


        if (FD_ISSET(listenfd, &read_set)) {

            if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) < 0) {
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
                } else if (request == 3) {
                    addExam(connfd);
                }
            }
        }
    }
}

    void addExamSession(int connfd) {
        MYSQL *conn = connection();

        char name[255] = {0};
        char date[12] = {0};

        // Leggi il nome dell'esame dalla socket
        read(connfd, name, sizeof(name));
        // Leggi la data dell'appello dalla socket
        read(connfd, date, sizeof(date));

        // Query parametrizzata per l'inserimento di un appello
        const char *query = "INSERT INTO exam_sessions (exam_session_name, session_date) VALUES (?, STR_TO_DATE(?, '%Y-%m-%d'))";

// Creazione dello statement preparato
        MYSQL_STMT *stmt = mysql_stmt_init(conn);
        if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
            const char *err = mysql_error(conn);
            write(connfd, err, strlen(err) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }

// Associazione dei parametri
        MYSQL_BIND params[2];
        memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_STRING;
        params[0].buffer = name;
        params[0].buffer_length = strlen(name);
        params[1].buffer_type = MYSQL_TYPE_STRING;
        params[1].buffer = date;
        params[1].buffer_length = strlen(date);

        if (mysql_stmt_bind_param(stmt, params) != 0) {
            const char *err = mysql_error(conn);
            write(connfd, err, strlen(err) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }

// Esecuzione dello statement
        if (mysql_stmt_execute(stmt) != 0) {
            if (strstr(mysql_error(conn), "foreign key constraint fails")) {
                const char *err = "non esiste un esame con questo nome!";
                write(connfd, err, strlen(err) + 1);
            } else {
                const char *err = mysql_error(conn);
                write(connfd, err, strlen(err) + 1);
            }
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }
        else{// Chiusura dello statement
            const char *ins = "inserimento del nuovo appello completato con successo!";
            write(connfd, ins, strlen(ins) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);}



    }




    void addExam(int connfd){
        MYSQL *conn = connection();
        char name[255] = {0};
        char corso[255]= {0};
        read(connfd, name, sizeof(name));
        read(connfd, corso, sizeof(corso));

        const char *query = "INSERT INTO exams (exam_name,nome_corso) VALUES (?,?)";

        MYSQL_STMT *stmt = mysql_stmt_init(conn);
        if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
            const char *err = mysql_error(conn);
            write(connfd, err, strlen(err) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }

        MYSQL_BIND params[2];
        memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_STRING;
        params[0].buffer = name;
        params[0].buffer_length = strlen(name);
        params[1].buffer_type = MYSQL_TYPE_STRING;
        params[1].buffer = corso;
        params[1].buffer_length = strlen(corso);

        if (mysql_stmt_bind_param(stmt, params) != 0) {
            const char *err = mysql_error(conn);
            write(connfd, err, strlen(err) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }


        if (mysql_stmt_execute(stmt) != 0) {
            const char *err = mysql_error(conn);
            write(connfd, err, strlen(err) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }else{// Chiusura dello statement
            const char *ins = "inserimento del nuovo esame completato con successo!";
            write(connfd, ins, strlen(ins) + 1);
            mysql_stmt_close(stmt);
            mysql_close(conn);}





    }


    void addBooking(int connfd) {

        MYSQL *conn = connection();

        char exam_name[255] , exam_date[255];
        // Leggi l'id dell'appello dalla socket
        read(connfd, exam_name, sizeof(exam_name));
        // Leggi la matricola dello studente dalla socket
        read(connfd, exam_date, sizeof(exam_date));

        // Query parametrizzata per l'inserimento di una prenotazione
        const char *query = "INSERT INTO booking (exam_name, date, progressive_number)\n"
                            "VALUES (?, STR_TO_DATE(?, '%Y-%m-%d'), (SELECT IFNULL(MAX(progressive_number), 0) + 1 FROM (SELECT * FROM booking) AS temp WHERE exam_name = ? AND date = STR_TO_DATE(?, '%Y-%m-%d')));";


        // Creazione dello statement preparato
        MYSQL_STMT *stmt = mysql_stmt_init(conn);
        if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore creazione statement");
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }

        // Associazione dei parametri
        MYSQL_BIND params[4];
        memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_STRING;
        params[0].buffer = exam_name;
        params[0].buffer_length = strlen(exam_name);
        params[1].buffer_type = MYSQL_TYPE_STRING;
        params[1].buffer = exam_date;
        params[1].buffer_length = strlen(exam_date);
        params[2].buffer_type = MYSQL_TYPE_STRING;
        params[2].buffer = exam_name;
        params[2].buffer_length = strlen(exam_name);
        params[3].buffer_type = MYSQL_TYPE_STRING;
        params[3].buffer = exam_date;
        params[3].buffer_length = strlen(exam_date);

        if (mysql_stmt_bind_param(stmt, params) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore biding");
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }

        // Esecuzione dello statement
        if (mysql_stmt_execute(stmt) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore esecuzione");
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return;
        }
        perror("esecuzione success");

        // Chiusura dello statement
        mysql_stmt_close(stmt);

        const char *ins = "Inserimento della nuova prenotazione completato con successo!";
        write(connfd, ins, strlen(ins) + 1);

        const char *max_query = "SELECT MAX(progressive_number) AS max_progressive_number FROM booking WHERE exam_name = ? AND date = ?";

        MYSQL_STMT *max_stmt = mysql_stmt_init(conn);
        if (mysql_stmt_prepare(max_stmt, max_query, strlen(max_query)) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore creazione statement");
            mysql_stmt_close(max_stmt);
            mysql_close(conn);
            return;
        }

        MYSQL_BIND max_params[2];
        memset(max_params, 0, sizeof(max_params));
        max_params[0].buffer_type = MYSQL_TYPE_STRING;
        max_params[0].buffer = exam_name;
        max_params[0].buffer_length = strlen(exam_name);
        max_params[1].buffer_type = MYSQL_TYPE_STRING;
        max_params[1].buffer = exam_date;
        max_params[1].buffer_length = strlen(exam_date);

        if (mysql_stmt_bind_param(max_stmt, max_params) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore biding");
            mysql_stmt_close(max_stmt);
            mysql_close(conn);
            return;
        }

        if (mysql_stmt_execute(max_stmt) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore esecuzione");
            mysql_stmt_close(max_stmt);
            mysql_close(conn);
            return;
        }

        MYSQL_BIND max_result;
        memset(&max_result, 0, sizeof(max_result));
        int max_progressive_number;
        max_result.buffer_type = MYSQL_TYPE_LONG;
        max_result.buffer = &max_progressive_number;

        if (mysql_stmt_bind_result(max_stmt, &max_result) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore biding risultato");
            mysql_stmt_close(max_stmt);
            mysql_close(conn);
            return;
        }

// Fetch del risultato
        if (mysql_stmt_fetch(max_stmt) != 0) {
            write(connfd, mysql_error(conn), strlen(mysql_error(conn)) + 1);
            perror("errore fetch risultato");
            mysql_stmt_close(max_stmt);
            mysql_close(conn);
            return;
        }
        mysql_stmt_close(max_stmt);


        write(connfd, &max_progressive_number, sizeof (max_progressive_number));


        mysql_close(conn);




    }

MYSQL *connection() {
    /**
     * Inizializzazione della connessione MySQL, con la funzione init che restituisce un puntatore a questa struttura.
     */
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() fallita\n");
        exit(1);
    }

    /**
     * Connessione vera e propria al database MySQL specificato da host, user, password e nome dello schema.
     */
    if (mysql_real_connect(conn, "87.11.19.92", "admin", "admin", "nuova_segreteria", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    return conn;
}




// printf("Hello, World!\n");
    //printf("test commit");
    //return 0;

