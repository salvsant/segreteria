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

    /**
     * Utilizzo la system call socket, che prende in input tre parametri di tipo intero, per creare una nuova socket
     * da associare al descrittore "listenfd". I tre parametri in input riguardano, in ordine, il dominio
     * degli indirizzi IP (IPv4 in questo caso), il protocollo di trasmissione (in questo caso TCP), mentre l'ultimo
     * parametro, se messo a 0, specifica che si tratta del protocollo standard.
     */
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }

    /**
     * Specifico la struttura dell'indirizzo del server tramite i campi di una struct di tipo sockaddr_in.
     * Vengono utilizzati indirizzi IPv4, vengono accettate connessioni da qualsiasi indirizzo e la porta su cui
     * il server risponderà ai client sarà la 1025.
     */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1025);

    /**
     * La system call bind permette di assegnare l'indirizzo memorizzato nel campo s_addr della struct sin_addr, che è
     * a sua volta un campo della struct sockaddr_in (servaddr nel nostro caso), al descrittore listenfd.
     */
    if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof (servaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        exit(1);
    }

    /**
     * Mettiamo il server in ascolto, specificando quante connessioni possono essere in attesa di venire accettate
     * tramite il secondo argomento della chiamata.
     */
    if ((listen(listenfd, 1024)) < 0) {
        perror("Errore nell'operazione di listen!");
        exit(1);
    }

    /**
     * Ci serviamo di un "insieme" di descrittori per strutturare la successiva select.
     * In particolare abbiamo read_set che mantiene l'insieme dei descrittori in lettura.
     * La variabile max_fd serve a specificare quante posizioni dell'array di descrittori devono essere controllate
     * all'interno della funzione select.
     */
    fd_set read_set;
    int max_fd;
    max_fd = listenfd;

    /**
     * Imposto un ciclo while "infinito" in modo che il server possa servire una nuova connessione,
     * quindi un nuovo client, dopo averne terminata una (di connessione client).
     */
    while(1) {
        /**
         * Ad ogni iterazione reinizializzo a 0 il read_set e vi aggiungo la socket che permette l'ascolto di nuove
         * connessioni da parte della segreteria.
         */
        FD_ZERO(&read_set);
        FD_SET(listenfd, &read_set);

        /**
         * Se il descrittore della socket relativa alla connessione con la segreteria è maggiore di -1 significa che la
         * segreteria è connessa e quindi si aggiunge anche il suo descrittore al read_set.
         */
        if (connfd > -1) {
            FD_SET(connfd, &read_set);
        }

        /**
         * La funzione select restituisce il numero di descrittori pronti.
         */
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("Errore nell'operazione di select!");
        }

        /**
         * Si controlla se sono in attesa di essere accettate nuove connessioni.
         */
        if (FD_ISSET(listenfd, &read_set)) {
            /**
             * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
             */
            if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                perror("Errore nell'operazione di accept!");
            }

            /**
             * Si ricalcola il numero di posizioni da controllare nella select
             */
            if (connfd > max_fd) {
                max_fd = connfd;
            }
        }

        /**
         * Si controlla se la segreteria vuole inviare una nuova richiesta al server universitario.
         */
        if (FD_ISSET(connfd, &read_set)) {
            /**
             * In caso affermativo si effettua la read, sempre se è possibile effettuarla, ossia se non viene
             * chiusa la segreteria.
             */
            if (read(connfd, &request, sizeof(request)) > 0) {
                /**
                 * Se la richiesta della segreteria è 1 si richiama la funzione di aggiunta di un appello, mentre se
                 * è 2 si richiama la funzione di aggiunta di una prenotazione di uno studente per un determinato
                 * appello.
                 */
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
}

/**
 * Procedura per l'aggiunta di un appello all'interno del database a partire dal nome dell'esame e dalla data passati
 * dalla segreteria.
 * @param connfd socket di connessione con la segreteria (client)
 */
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
    perror("test");
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


/**
 * Procedura per l'aggiunta di una prenotazione ad un appello da parte di uno studente all'interno del database,
 * a partire dall'id dell'appello, dalla matricola dello studente che si sta prenotando e dalla data nella quale avviene
 * la prenotazione.
 * @param connfd socket di connessione con la segreteria (client)
 */
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
    if (mysql_real_connect(conn, "95.252.224.133", "admin", "admin", "nuova_segreteria", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() fallita: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    return conn;
}

