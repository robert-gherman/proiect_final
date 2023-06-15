#include <stdio.h>     // Biblioteca standard pentru operații de intrare/ieșire
#include <stdlib.h>    // Biblioteca standard pentru funcții generale de utilitate
#include <string.h>    // Biblioteca standard pentru operații cu șiruri de caractere
#include <unistd.h>    // Biblioteca pentru funcții de sistem legate de descriptori de fișiere
#include <dirent.h>    // Biblioteca pentru manipularea directoriilor
#include <sys/stat.h>  // Biblioteca pentru manipularea informațiilor despre fișiere
#include <arpa/inet.h> // Biblioteca pentru operații de rețea la nivel de socket

#define SERVER_IP "127.0.0.1"
#define PORT 8016
#define MAX_BUFFER_SIZE 8192

// Funcție pentru conectarea la server
int connect_to_server()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Creare socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertirea adreselor IP din forma text în formă binară
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Conectarea la server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        return -1;
    }

    return sock;
}

// Funcție pentru trimiterea unui mesaj la server
void send_message(int sock, const char *message)
{
    send(sock, message, strlen(message), 0);
    printf("Message sent to server: %s\n", message);
}

// Funcție pentru primirea unui mesaj de la server
char *receive_message(int sock)
{
    char *buffer = (char *)malloc(MAX_BUFFER_SIZE * sizeof(char));
    if (buffer == NULL)
    {
        perror("Memory allocation failed");
        exit(1);
    }

    memset(buffer, 0, MAX_BUFFER_SIZE); // Golirea bufferului

    int valread = read(sock, buffer, MAX_BUFFER_SIZE - 1);
    printf("Received message from server: %s\n", buffer);

    return buffer;
}

// Funcție pentru primirea unui fișier de la server
void receive_file(int sock, char *username, char *item)
{
    FILE *file;
    char buffer[MAX_BUFFER_SIZE] = {0};

    // Verificarea dacă directorul există deja
    struct stat st;
    if (stat("downloads", &st) == 0 && S_ISDIR(st.st_mode))
    {
        printf("Directory '%s' already exists.\n", "downloads");
    }
    else
    {
        // Crearea directorului părinte
        int result = mkdir("downloads", 0700);
        if (result == 0)
        {
            printf("Parent directory '%s' created successfully.\n", "downloads");
        }
        else
        {
            printf("Failed to create parent directory '%s'.\n", "downloads");
        }
    }

    struct stat st1;
    char dir[1028];
    sprintf(dir, "downloads/%s", username);
    if (stat(dir, &st1) == 0 && S_ISDIR(st1.st_mode))
    {
        printf("Directory '%s' already exists.\n", dir);
    }
    else
    {
        // Crearea directorului părinte
        int result = mkdir(dir, 0700);
        if (result == 0)
        {
            printf("Parent directory '%s' created successfully.\n", dir);
        }
        else
        {
            printf("Failed to create parent directory '%s'.\n", dir);
        }
    }

    sprintf(dir, "downloads/%s/%s", username, item);

    // Deschiderea fișierului pentru a scrie datele primite
    file = fopen(dir, "wb");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Primirea și scrierea conținutului fișierului în fragmente
    ssize_t bytes_received, bytes_written;
    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        bytes_written = fwrite(buffer, sizeof(char), bytes_received, file);
    }

    // Verificarea dacă a apărut o eroare în timpul primirii fișierului
    if (bytes_received < 0)
    {
        perror("Error receiving file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Închiderea fișierului
    fclose(file);

    printf("File received successfully.\n");
    exit(EXIT_SUCCESS);
}
