#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8081

int client(char buffer[BUFFER_SIZE]) {
    int client_fd;
    struct sockaddr_in server_addr;

    // Create a TCP socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert the server IP address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");

    // Read input from the user and send it to the server
    // while (1) {
        printf("Enter a message (or 'quit' to exit): ");
        // fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove the newline character

        // Send the message to the server
        ssize_t num_bytes = send(client_fd, buffer, strlen(buffer), 0);
        if (num_bytes == -1) {
            perror("Send failed");
            // break;
        }

        // Check if the user wants to quit
        if (strcmp(buffer, "quit") == 0) {
            // break;
        }

        // Receive and display the server's response
        num_bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (num_bytes == -1) {
            perror("Receive failed");
            // break;
        }

        buffer[num_bytes] = '\0';
        printf("Server response: %s\n", buffer);
    // }

    // Close the client socket
    // close(client_fd);

    return 0;
}
