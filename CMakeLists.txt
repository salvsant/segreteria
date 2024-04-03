cmake_minimum_required(VERSION 3.22.1)
project(ProgettoSegreteria C)

set(CMAKE_C_STANDARD 11)

#Trova le librerie MySQL (per mac - )
find_library(MYSQL_LIBRARY_PATH mysqlclient HINTS "/usr/lib/x86_64-linux-gnu/")

# Trova i file di intestazione di MySQL
find_path(MYSQL_INCLUDE_DIR mysql.h HINTS "/usr/include/mysql")

# Aggiungi gli eseguibili del progetto
add_executable(server_exec server.c)
add_executable(segretaria_exec segreteria.c)
add_executable(studente_exec studente.c)

# Collega i tuoi eseguibili alle librerie di MySQL
target_link_libraries(server_exec PRIVATE ${MYSQL_LIBRARY_PATH})
target_link_libraries(segretaria_exec PRIVATE ${MYSQL_LIBRARY_PATH})
target_link_libraries(studente_exec PRIVATE ${MYSQL_LIBRARY_PATH})

# Aggiungi la directory dei file di intestazione di MySQL al tuo progetto
target_include_directories(server_exec PRIVATE ${MYSQL_INCLUDE_DIR})
target_include_directories(segretaria_exec PRIVATE ${MYSQL_INCLUDE_DIR})
target_include_directories(studente_exec PRIVATE ${MYSQL_INCLUDE_DIR})
