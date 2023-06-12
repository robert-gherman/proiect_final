#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9999
#define MAX_BUFFER_SIZE 1024

 int connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    return sock;
}

void send_message(int sock, const char *message) {
    send(sock, message, strlen(message), 0);
    printf("Message sent to server: %s\n", message);
}

char* receive_message(int sock) {
    char buffer[MAX_BUFFER_SIZE] = {0};
    int valread = read(sock, buffer, sizeof(buffer));
    printf("Received message from server: %s\n", buffer);
    return buffer;
}

// int main() {
//     int sock = connect_to_server();
//     if (sock == -1) {
//         printf("Failed to connect to the server\n");
//         return 1;
//     }

//     send_message(sock, "Hello, server!");
//     receive_message(sock);

//     close(sock);
//     return 0;
// }
