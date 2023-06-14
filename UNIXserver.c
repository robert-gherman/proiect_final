#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <jansson.h>

#define MAX_CLIENTS 1
#define BUFFER_SIZE 1024
#define SOCKET_PATH "/tmp/socket2"

// Structure to hold client information
typedef struct
{
    int client_fd;
} client_info_t;

json_t *load_json_from_file(const char *filename)
{
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root)
    {
        fprintf(stderr, "Error loading JSON file: %s\n", error.text);
    }
    return root;
}

size_t get_json_array_length(json_t *root)
{
    if (!json_is_array(root))
    {
        fprintf(stderr, "JSON root is not an array\n");
        return 0;
    }

    return json_array_size(root);
}

void add_json_object(json_t *root, json_t *new_object)
{
    if (!json_is_array(root))
    {
        fprintf(stderr, "JSON root is not an array\n");
        return;
    }

    json_array_append(root, new_object);
}

int verify_credentials(json_t *root, const char *username, const char *password)
{
    if (!json_is_array(root))
    {
        fprintf(stderr, "JSON root is not an array\n");
        return 0;
    }

    size_t i;
    json_t *value;
    json_array_foreach(root, i, value)
    {
        json_t *username_value = json_object_get(value, "username");
        json_t *password_value = json_object_get(value, "password");

        if (json_is_string(username_value) && json_is_string(password_value))
        {
            const char *stored_username = json_string_value(username_value);
            const char *stored_password = json_string_value(password_value);

            if (strcmp(stored_username, username) == 0 && strcmp(stored_password, password) == 0)
            {
                return 1; // Credentials match
            }
        }
    }

    return 0; // Credentials not found
}

// Write JSON to file
int write_json_to_file(const char *filename, json_t *root)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        fprintf(stderr, "Error opening file for writing\n");
        return 0;
    }

    int success = json_dumpf(root, file, JSON_INDENT(4));
    fclose(file);

    return success == 0;
}

// Function to handle communication with a client
void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    int client_fd = client->client_fd;

    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;

    // Read client messages until 'quit' is received
    while ((num_bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[num_bytes] = '\0';
        printf("Received from client: %s\n", buffer);

        // First call to strtok
        char *token = strtok(buffer, ":");
        printf("%s\n", token);
        if (strcmp(token, "register") == 0)
        {
            char *username;
            char *email;
            char *password;
            // Subsequent calls to strtok
            while (token != NULL)
            {
                if (strcmp(token, "username") == 0)
                {
                    token = strtok(NULL, ":");
                    printf("%s\n", token);
                    username = token;
                }
                if (strcmp(token, "password") == 0)
                {
                    token = strtok(NULL, ":");
                    printf("%s\n", token);
                    password = token;
                }
                if (strcmp(token, "email") == 0)
                {
                    token = strtok(NULL, ":");
                    printf("%s\n", token);
                    email = token;
                }
                token = strtok(NULL, ":");
                // printf("Token: %s\n", token);
            }

            json_t *root = load_json_from_file("credentials.json");
            if (!root)
            {
                // Handle the error
                exit(EXIT_FAILURE);
            }

            json_t *new_object = json_pack("{s:s, s:s, s:s}", "username", username, "password", password, "email", email);
            add_json_object(root, new_object);
            if (write_json_to_file("credentials.json", root))
            {
                printf("JSON data written to file successfully.\n");
            }
            else
            {
                fprintf(stderr, "Error writing JSON data to file.\n");
            }

            json_decref(root);

            char dir[256];
            sprintf(dir, "%s/%s", "./drive", username);
            mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

            send(client_fd, "registered", strlen("registered"), 0);
        }

        if (strcmp(token, "download") == 0)
        {
            token = strtok(NULL, ":");
            char *username = token;
            printf("%s\n", token);

            token = strtok(NULL, ":");
            printf("%s\n", token);

            char dir[1024];
            sprintf(dir, "drive/%s/%s", username, token);
            // Open file
            FILE *file = fopen(dir, "rb");
            if (file == NULL)
            {
                perror("Failed to open file");
                exit(EXIT_FAILURE);
            }

            // Read and send file contents in chunks
            ssize_t bytes_read;
            while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0)
            {
                ssize_t bytes_sent = send(client_fd, buffer, bytes_read, 0);
                printf("%s\n", buffer);
                if (bytes_sent == -1)
                {
                    perror("Failed to send data");
                    exit(EXIT_FAILURE);
                }
            }

            // Close file
            fclose(file);
        }
    }

    // Close the client socket
    close(client_fd);
    printf("Client disconnected\n");

    free(client);
    pthread_exit(NULL);
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len;

    // Create a UNIX domain socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) == -1)
    {
        perror("Socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on UNIX socket %s\n", SOCKET_PATH);

    // Accept and handle client connections
    while (1)
    {
        client_addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1)
        {
            perror("Failed to accept client connection");
            continue;
        }

        printf("Client connected\n");

        // Create a new thread to handle the client
        pthread_t tid;
        client_info_t *client_info = (client_info_t *)malloc(sizeof(client_info_t));
        client_info->client_fd = client_fd;

        if (pthread_create(&tid, NULL, handle_client, (void *)client_info) != 0)
        {
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

    // Remove the UNIX socket file
    unlink(SOCKET_PATH);

    return 0;
}
