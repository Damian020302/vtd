#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 7006        // Puerto en el que el servidor escucha
#define BUFFER_SIZE 1024 // Tamaño del buffer para recibir datos

void decryptCaesar(char *text, int shift) {
  shift = shift % 26;
  for (int i = 0; text[i] != '\0'; i++) {
    char c = text[i];
    if (isupper(c)) {
      text[i] = ((c - 'A' - shift + 26) % 26) + 'A';
    } else if (islower(c)) {
      text[i] = ((c - 'a' - shift + 26) % 26) + 'a';
    } else {
      text[i] = c;
    }
  }
}

void saveNetworkInfo(const char *outputFile) {
  FILE *fpCommand;
  FILE *fpOutput;
  char buffer[512];

  // Ejecutar comando para obtener información de red
  fpCommand = popen("ip addr show", "r");
  if (fpCommand == NULL) {
    perror("Error!");
    return;
  }

  // Abrir archivo para guardar la salida
  fpOutput = fopen(outputFile, "w");
  if (fpOutput == NULL) {
    perror("[-] Error to open the file");
    pclose(fpCommand);
    return;
  }

  // Leer línea por línea y escribir en el archivo
  while (fgets(buffer, sizeof(buffer), fpCommand) != NULL) {
    fputs(buffer, fpOutput);
  }

  // Cerrar ambos archivos
  fclose(fpOutput);
  pclose(fpCommand);
}

void saveSystemInfo(const char *outputFile) {
  FILE *fp = fopen(outputFile, "w");
  if (fp == NULL) {
    perror("[-] Error creating system info file");
    return;
  }

  fprintf(fp, "=== INFORMACIÓN DEL SISTEMA ===\n\n");

  // OS y Kernel
  fprintf(fp, "--- OS Y KERNEL ---\n");
  FILE *cmd = popen("uname -a", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Distribución
  fprintf(fp, "--- DISTRIBUCIÓN ---\n");
  cmd = popen("lsb_release -a 2>/dev/null", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // IPs
  fprintf(fp, "--- DIRECCIONES IP ---\n");
  cmd = popen("hostname -I", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "IP Local: %s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // CPU Info
  fprintf(fp, "--- INFORMACIÓN DE CPU ---\n");
  cmd = popen("grep 'model name' /proc/cpuinfo", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "CPU:%s", buffer);
    }
    pclose(cmd);
  }

  // Núcleos
  fprintf(fp, "--- NÚCLEOS ---\n");
  cmd = popen("nproc", "r");
  if (cmd) {
    char buffer[64];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "Núcleos: %s", buffer);
    }
    pclose(cmd);
  }

  // Arquitectura
  fprintf(fp, "--- ARQUITECTURA ---\n");
  cmd = popen("uname -m", "r");
  if (cmd) {
    char buffer[64];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "Arquitectura: %s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Memoria
  fprintf(fp, "--- MEMORIA ---\n");
  cmd = popen("free -h", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Disco
  fprintf(fp, "--- ESPACIO EN DISCO ---\n");
  cmd = popen("df -h", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Usuarios conectados
  fprintf(fp, "--- USUARIOS CONECTADOS ---\n");
  cmd = popen("who", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Todos los usuarios del sistema
  fprintf(fp, "--- TODOS LOS USUARIOS ---\n");
  cmd = popen("cut -d: -f1 /etc/passwd", "r");
  if (cmd) {
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Uptime
  fprintf(fp, "--- TIEMPO DE ACTIVIDAD ---\n");
  cmd = popen("uptime", "r");
  if (cmd) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Procesos activos
  fprintf(fp, "--- PROCESOS ACTIVOS (TOP 10) ---\n");
  cmd = popen("ps aux --sort=-%cpu | head -11", "r");
  if (cmd) {
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }
  fprintf(fp, "\n");

  // Directorios montados
  fprintf(fp, "--- SISTEMAS DE ARCHIVOS MONTADOS ---\n");
  cmd = popen("mount | grep '^/'", "r");
  if (cmd) {
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), cmd)) {
      fprintf(fp, "%s", buffer);
    }
    pclose(cmd);
  }

  fclose(fp);
  printf("[+] System information saved to %s\n", outputFile);
}

void sendFile(const char *filename, int sockfd) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("[-] Cannot open the file");
    return;
  }
  char buffer[BUFFER_SIZE];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    if (send(sockfd, buffer, bytes, 0) == -1) {
      perror("[-] Error to send the file");
      break;
    }
  }
  fclose(fp);
}

// Función para convertir a minúsculas
void toLowerCase(char *str) {
  for (int i = 0; str[i]; i++)
    str[i] = tolower((unsigned char)str[i]);
}

// Función para eliminar espacios al inicio y final
void trim(char *str) {
  char *end;
  while (isspace((unsigned char)*str))
    str++; // inicio
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--; // final
  *(end + 1) = '\0';
}

bool isOnFile(const char *bufferOriginal) {
  FILE *fp;
  char line[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  bool foundWorld = false;

  // Copiamos el buffer original para poder modificarlo
  strncpy(buffer, bufferOriginal, BUFFER_SIZE);
  buffer[BUFFER_SIZE - 1] = '\0'; // aseguramos terminación

  trim(buffer);
  toLowerCase(buffer);

  fp = fopen("cipherworlds.txt", "r");
  if (fp == NULL) {
    printf("[-]Error opening file!\n");
    return false;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    line[strcspn(line, "\n")] = '\0';
    trim(line);
    toLowerCase(line);
    if (strcmp(line, buffer) == 0) {
      foundWorld = true;
      break;
    }
  }

  fclose(fp);
  return foundWorld;
}

int main() {
  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  char buffer[BUFFER_SIZE] = {0};
  char clave[BUFFER_SIZE];
  int shift;

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock == -1) {
    perror("[-] Error to create the socket");
    return 1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("[-] Error binding");
    close(server_sock);
    return 1;
  }
  
  if (listen(server_sock, 1) < 0) {
    perror("[-] Error on listen");
    close(server_sock);
    return 1;
  }

  printf("[+] Server listening port %d...\n", PORT);

  addr_size = sizeof(client_addr);
  client_sock =
      accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
  if (client_sock < 0) {
    perror("[-] Error on accept");
    close(server_sock);
    return 1;
  }

  printf("[+] Client connected\n");

  int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
  if (bytes <= 0) {
    printf("[-] Missed key\n");
    close(client_sock);
    close(server_sock);
    return 1;
  }

  buffer[bytes] = '\0';
  sscanf(buffer, "%s %d", clave, &shift); // extrae clave y desplazamiento
  printf("[+][Server] Encrypted key obtained: %s\n", clave);

  if (isOnFile(clave)) {
    decryptCaesar(clave, shift);
    printf("[+][Server] Key decrypted: %s\n", clave);
    send(client_sock, "ACCESS GRANTED", strlen("ACCESS GRANTED"), 0);
    sleep(1); // Pequeña pausa para evitar colisión de datos
    
    // Generar información del sistema
    saveSystemInfo("sysinfo.txt");
    sendFile("sysinfo.txt", client_sock);
    printf("[+] Sent system info file\n");
    
    saveNetworkInfo("network_info.txt");
    sendFile("network_info.txt", client_sock);
    printf("[+] Sent network info file\n");
  } else {
    send(client_sock, "ACCESS DENIED", strlen("ACCESS DENIED"), 0);
    printf("[-][Server] Wrong Key\n");
  }
  close(client_sock);
  close(server_sock);
  return 0;
}