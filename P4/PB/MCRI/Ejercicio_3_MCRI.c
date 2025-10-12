#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <time.h>

#define MAIN_PORT 49200
#define BUFFER_SIZE 1024

char **alias_list;
int num_alias;

void get_datetime(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int bytes;

    // Recibir mensajes
    while ((bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        char datetime[64];
        get_datetime(datetime, sizeof(datetime));
        printf("[%s] Mensaje recibido: %s\n", datetime, buffer);
        fflush(stdout);
    }

    close(client_sock);
    pthread_exit(NULL);
}

void *server_thread(void *arg) {
    char *alias = (char *)arg;
    struct addrinfo hints, *res;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", MAIN_PORT);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Obtneer dirección IP del alias
    if (getaddrinfo(alias, port_str, &hints, &res) != 0) {
        perror("[-] Error al resolver alias");
        pthread_exit(NULL);
    }

    // Crear socket
    int server_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_sock < 0) {
        perror("[-] Error al crear socket servidor");
        freeaddrinfo(res);
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Enlazar socket
    if (bind(server_sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("[-] Error en bind");
        close(server_sock);
        freeaddrinfo(res);
        pthread_exit(NULL);
    }

    freeaddrinfo(res);

    // Escuchar conexiones
    if (listen(server_sock, 10) < 0) {
        perror("[-] Error en listen");
        close(server_sock);
        pthread_exit(NULL);
    }

    printf("[+] Servidor en alias '%s' escuchando en puerto %d...\n", alias, MAIN_PORT);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int *client_sock = malloc(sizeof(int));

        // Aceptar conexión
        if ((*client_sock = accept(server_sock, (struct sockaddr *)&cli_addr, &cli_len)) < 0) {
            perror("[-] Error en accept");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);
    }

    close(server_sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <alias1> <alias2> ... <aliasN>\n", argv[0]);
        return 1;
    }

    num_alias = argc - 1;
    alias_list = &argv[1];

    pthread_t threads[num_alias];

    for (int i = 0; i < num_alias; i++) {
        // Crear hilo para cada alias
        if (pthread_create(&threads[i], NULL, server_thread, alias_list[i]) != 0) {
            perror("[-] Error al crear hilo de servidor");
            return 1;
        }
    }

    for (int i = 0; i < num_alias; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
