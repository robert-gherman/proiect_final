#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024
#define PORT 8081

// Structure to hold client information
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

// Function to handle communication with a client
void *handle_client(void *arg) {
    client_info_t *client = (client_info_t *)arg;
    int client_fd = client->client_fd;
    struct sockaddr_in client_addr = client->client_addr;

    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;

    // Read client messages until 'quit' is received
    while ((num_bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[num_bytes] = '\0';
        printf("Received from client %s:%d: %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), buffer);

        // Check if the client wants to quit
        if (strcmp(buffer, "quit") == 0) {
            break;
        }

        // Echo back the received message
        send(client_fd, buffer, strlen(buffer), 0);
    }

    // Close the client socket
    close(client_fd);
    printf("Client %s:%d disconnected\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    free(client);
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("Socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", ntohs(server_addr.sin_port));

    // Accept and handle client connections
    while (1) {
        client_addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            perror("Failed to accept client connection");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Create a new thread to handle the client
        pthread_t tid;
        client_info_t *client_info = (client_info_t *)malloc(sizeof(client_info_t));
        client_info->client_fd = client_fd;
        client_info->client_addr = client_addr;

        if (pthread_create(&tid, NULL, handle_client, (void *)client_info) != 0) {
            perror("Failed to create thread");
            free(client_info);
            close(client_fd);
            continue;
        }

        // Detach the thread so that it can clean up resources when it exits
        pthread_detach(tid);
    }

    // Close the server socket
    close(server_fd);

    return 0;
}
