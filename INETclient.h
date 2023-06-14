#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#define SERVER_IP "127.0.0.1"
#define PORT 2021
#define MAX_BUFFER_SIZE 1024

int connect_to_server()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        return -1;
    }

    return sock;
}

void send_message(int sock, const char *message)
{
    send(sock, message, strlen(message), 0);
    printf("Message sent to server: %s\n", message);
}

char *receive_message(int sock)
{
    char buffer[MAX_BUFFER_SIZE] = {0};
    int valread = read(sock, buffer, sizeof(buffer));
    printf("Received message from server: %s\n", buffer);
    return buffer;
}

char *receive_file(int sock, char *username, char *item)
{
    FILE *file;
    char buffer[MAX_BUFFER_SIZE];

    // Check if the directory already exists
    struct stat st;
    if (stat("downloads", &st) == 0 && S_ISDIR(st.st_mode))
    {
        printf("Directory '%s' already exists.\n", "downloads");
    }
    else
    {
        // Create the parent directory
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
        // Create the parent directory
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
    // printf("%s\n", dir);
    // Open file to write received data
    file = fopen(dir, "wb");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Receive and write file contents in chunks
    ssize_t bytes_received;
    while ((bytes_received = recv(sock, buffer, MAX_BUFFER_SIZE, 0)) > 0)
    {
        ssize_t bytes_written = fwrite(buffer, sizeof(char), bytes_received, file);
        printf("%s\n", buffer);
        if (bytes_written < bytes_received)
        {
            perror("Failed to write to file");
            exit(EXIT_FAILURE);
        }
    }
    printf("File received successfully.\n");
    // Close file
    fclose(file);
}
