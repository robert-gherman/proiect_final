#include <stdio.h>      //  pentru input/output
#include <stdlib.h>     //  pentru functii generale
#include <string.h>     //  pentru functii cu string-uri
#include <sys/socket.h> //  pentru comunicare prin socket
#include <netinet/in.h> //  pentru adrese IP
#include <arpa/inet.h>  //  pentru convertirea adresei IP
#include <unistd.h>     //  pentru functii de sistem
#include <time.h>       //  pentru functii cu timp
#include <pwd.h>        //  pentru informatii despre utilizatori
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <jansson.h>

#define MAX_CLIENTS 2
#define BUFFER_SIZE 8192
#define PORT 8035

// Structure to hold client information
typedef struct
{   char* username;
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

client_info_t clients[MAX_CLIENTS];
int num_clients = 0;

void disconnectClient(const char* username) {
    int i;
    for (i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].username, username) == 0) {
            int clientSocket = clients[i].client_fd;
            close(clientSocket);
            printf("Client disconnected: %s (%s:%d)\n", username,
                   inet_ntoa(clients[i].client_addr.sin_addr),
                   ntohs(clients[i].client_addr.sin_port));
            break;
        }
    }

    if (i < num_clients - 1) {
        // Remove the client from the array by shifting the remaining clients
        memmove(&clients[i], &clients[i + 1], (num_clients - i - 1) * sizeof(client_info_t));
    }

    num_clients--;
}

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

int deleteJsonObjectFromArray(json_t *array, const char *username)
{
    size_t index;
    json_t *value;

    json_array_foreach(array, index, value)
    {
        const char *jsonUsername = json_string_value(json_object_get(value, "username"));
        if (jsonUsername && strcmp(jsonUsername, username) == 0)
        {
            json_array_remove(array, index);
            return 1; // Object deleted successfully
        }
    }

    return 0; // Object not found
}

void deleteFile(const char *directory, const char *filename)
{
    // Create the full file path by concatenating the directory and filename
    size_t directoryLen = strlen(directory);
    size_t filenameLen = strlen(filename);
    size_t pathLen = directoryLen + 1 + filenameLen + 1; // +1 for directory separator, +1 for null terminator
    char *filePath = (char *)malloc(pathLen);
    snprintf(filePath, pathLen, "%s/%s", directory, filename);

    // Delete the file
    if (remove(filePath) == 0)
    {
        printf("File '%s' deleted successfully.\n", filePath);
    }
    else
    {
        printf("Error deleting file '%s'.\n", filePath);
    }

    // Cleanup
    free(filePath);
}
// Function to handle communication with a client
void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    int client_fd = client->client_fd;
    struct sockaddr_in client_addr = client->client_addr;
    char* username = client->username; 

    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;

    // Read client messages until 'quit' is received
    while ((num_bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[num_bytes] = '\0';
        printf("Received from client %s:%d: %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), buffer);

        // First call to strtok
        char *token = strtok(buffer, ":");
        printf("%s\n", token);

        if (strcmp(token, "disconnect") == 0)
        {
            token = strtok(NULL, ":");
            printf("%s\n", token);
            disconnectClient(token);
            send(client_fd, "DISCONNECTED", strlen("DISCONNECTED"), 0);
        }

        if (strcmp(token, "deleteFILE") == 0)
        {
            token = strtok(NULL, ":");
            printf("%s\n", token);
            char username[BUFFER_SIZE] = "";
            char filename[BUFFER_SIZE] = "";
            while (token != NULL)
            {
                if (strcmp(token, "username") == 0)
                {
                    token = strtok(NULL, ":");
                    printf("%s\n", token);
                    strcpy(username, token);
                    token = strtok(NULL, ":");
                    printf("%s\n", token);
                    strcpy(filename, token);
                }
                token = strtok(NULL, ":");
            }
            char filePath[BUFFER_SIZE];
            sprintf(filePath, "drive/%s", username);
            deleteFile(filePath, filename);
            send(client_fd, "DELETED", strlen("DELETED"), 0);
        }

        if (strcmp(token, "deleteUSER") == 0)
        {
            token = strtok(NULL, ":");
            printf("%s\n", token);
            // Read JSON array from file
            FILE *file = fopen("credentials.json", "r");
            json_error_t error;
            json_t *root = json_loadf(file, 0, &error);
            fclose(file);

            if (!root)
            {
                fprintf(stderr, "Error reading JSON file: %s\n", error.text);
            }

            // Delete JSON object with username "qwe"
            int deleted = deleteJsonObjectFromArray(root, token);

            if (deleted)
            {
                // Write modified JSON array back to file
                FILE *outfile = fopen("credentials.json", "w");
                json_dumpf(root, outfile, JSON_INDENT(4));
                fclose(outfile);
                printf("Object with username \"%s\" deleted successfully.\n", token);
            }
            else
            {
                printf("Object with username \"%s\" not found.\n", token);
            }

            // Cleanup
            json_decref(root);
            send(client_fd, "DELETED", strlen("DELETED"), 0);
        }

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

        if (strcmp(token, "login") == 0)
        {
            token = strtok(NULL, ":");

            char *username;
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

                token = strtok(NULL, ":");
                // printf("Token: %s\n", token);
            }

            json_t *root = load_json_from_file("credentials.json");
            if (!root)
            {
                // Handle the error
                exit(EXIT_FAILURE);
            }

            if (verify_credentials(root, username, password) == 1)
            {
                if (strcmp(username, "admin") == 0)
                {
                    printf("ADMIN\n");
                    send(client_fd, "ADMIN", strlen("ADMIN"), 0);
                }
                else
                {
                    send(client_fd, "OK", strlen("OK"), 0);
                }
            }
            else
            {
                send(client_fd, "NO", strlen("NO"), 0);
            }
        }
    }

    // Close the client socket
    close(client_fd);
    printf("Client %s:%d disconnected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    num_clients--;
    printf("Connected clients: %d\n", num_clients);
    free(client);
    pthread_exit(NULL);
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
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

    printf("Server listening on port %d\n", ntohs(server_addr.sin_port));

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

        // Check if maximum number of clients reached
        if (num_clients >= MAX_CLIENTS)
        {
            printf("Maximum number of clients reached. Rejecting new connection.\n");
            close(client_fd);
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Create a new client_info structure and allocate memory
        client_info_t *client_info = malloc(sizeof(client_info_t));
        client_info->client_fd = client_fd;
        client_info->client_addr = client_addr;
        // client_info->username = username;

        // Add client to the list
        clients[num_clients] = *client_info;
        num_clients++;
        printf("Connected clients: %d\n", num_clients);
        // Create a new thread to handle the client
        pthread_t tid;
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

    return 0;
}