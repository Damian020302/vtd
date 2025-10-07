#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024
int ports[] = {49200, 49201, 49202};

int send_request(const char *server_ip, int port, int target_port, int shift, char *filename)
{
    int client_sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    int bytes;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1)
    {
        perror("[-] Error al crear el socket");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        perror("[-] Dirección inválida/ Dirección no soportada");
        close(client_sock);
        return 1;
    }

    if (connect(client_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("[-] Error al conectar al servidor");
        close(client_sock);
        return 1;
    }

    printf("[+] Conectado al servidor\n");

    // Enviar puerto objetivo
    int net_port = htonl(target_port);
    if (send(client_sock, &net_port, sizeof(net_port), 0) == -1)
    {
        perror("[-] Error enviando puerto objetivo");
        close(client_sock);
        return 1;
    }

    // Enviar desplazamiento
    int net_shift = htonl(shift);
    if (send(client_sock, &net_shift, sizeof(net_shift), 0) == -1)
    {
        perror("[-] Error enviando desplazamiento");
        close(client_sock);
        return 1;
    }

    // Enviar nombre del archivo
    size_t fname_len = strlen(filename) + 1; // +1 para el terminador nulo
    if (send(client_sock, filename, fname_len, 0) == -1)
    {
        perror("[-] Error enviando nombre del archivo");
        close(client_sock);
        return 1;
    }

    // Enviar el archivo
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("[-] Error al abrir el archivo");
        close(client_sock);
        return 1;
    }

    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        if (send(client_sock, buffer, bytes, 0) == -1)
        {
            perror("[-] Error al enviar el archivo");
            fclose(fp);
            close(client_sock);
            return 1;
        }
    }
    fclose(fp);

    shutdown(client_sock, SHUT_WR); // Indicar que no se enviarán más datos
    printf("[+][Client] Enviado puerto=%d, shift=%d y archivo='%s'\n", target_port, shift, filename);

    // Esperar respuesta del servidor
    bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        printf("[-] No se recibio respuesta del servidor\n");
        close(client_sock);
        return 1;
    }

    printf("[+][Client] Mensaje del servidor: %s\n", buffer);
    buffer[bytes] = '\0';

    if (strstr(buffer, "ACCESO OTORGADO") != NULL)
    {
        char output[256];
        snprintf(output, sizeof(output), "cifrado_%d_%s", port, filename);

        FILE *out = fopen(output, "w");
        if (out == NULL)
        {
            perror("[-] Error to open the file");
            close(client_sock);
            return 1;
        }

        while ((bytes = recv(client_sock, buffer, sizeof(buffer), 0)) > 0)
        {
            fwrite(buffer, 1, bytes, out);
        }

        fclose(out);
        printf("[+][Client] El archivo fue guardado como 'cifrado_%d_%s.txt'\n", port);
    }
    else
    {
        printf("[-] No se recibio respuesta del servidor\n");
    }

    close(client_sock);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Uso: %s <SERVIDOR_IP> <PUERTO> <DESPLAZAMIENTO> <Nombre del archivo>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int target_port = atoi(argv[2]);
    int shift = atoi(argv[3]);
    char *filename = argv[4];
    int n_ports = sizeof(ports) / sizeof(ports[0]);

    for (int i = 0; i < n_ports; i++)
    {
        send_request(server_ip, ports[i], target_port, shift, filename);
    }
    return 0;
}
