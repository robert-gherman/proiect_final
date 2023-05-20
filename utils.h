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

#define SERVER_ADDR "127.0.0.1" // adresa IP a serverului
#define SERVER_PORT 8080        // portul la care serverul asculta conexiuni
#define MESSAGE_LEN 1024        // dimensiunea maxima a mesajelor
#define BUFSIZE 4096

typedef struct inFile
{
    char *filePath;
    char *buffer;
    size_t size;
} inFile_t;

typedef struct outFile
{
    char *filePath;
    char *buffer;
    size_t size;
} outFile_t;

// functie pentru timp
void get_time(char *response)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(response, MESSAGE_LEN, "%a %d %b %H:%M:%S", timeinfo);
}
// finctie pentru user info
void get_user(char *response)
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        snprintf(response, MESSAGE_LEN, "%s", pw->pw_name);
    }
    else
    {
        snprintf(response, MESSAGE_LEN, "unknown");
    }
}

void TCP_server()
{

    // Se creeaza un socket pentru server
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // tratam cazuri de eroare
    if (server_fd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    // tratam cazuri de eroare
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configureaza adresa serverului
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr)); // Initializeaza structura cu 0
    serv_addr.sin_family = AF_INET;           // Adresa este de tip IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;   // Adresa IP este locala
    serv_addr.sin_port = htons(SERVER_PORT);  // Portul serverului
                                              // tratam cazuri de eroare
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Asculta conexiunile venite catre server
    if (listen(server_fd, 1) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Accepta o conexiune de la un client
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int cli_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
    if (cli_fd < 0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    // Asteapta mesaje de la client
    char message[MESSAGE_LEN];
    while (1)
    {
        int bytes_read = read(cli_fd, message, MESSAGE_LEN);
        // tratam cazuri de eroare
        if (bytes_read < 0)
        {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
        message[bytes_read] = '\0';

        char response[MESSAGE_LEN];
        // verificam  daca clientul a trimis un mesaj time, user, sau un simplu mesaj
        if (strcmp(message, "time\n") == 0)
        {
            get_time(response);
            printf("Sending current time to client...\n");
        }
        else if (strcmp(message, "user\n") == 0)
        {
            get_user(response);
            printf("Sending current user to client...\n");
        }
        else
        {
            snprintf(response, MESSAGE_LEN, "echo :: %s", message);
            printf("Sending some echo to client...\n");
        }
        // tratam cazuri de eroare
        if (send(cli_fd, response, strlen(response), 0) < 0)
        {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
    }

    close(cli_fd);
    close(server_fd);
}

void TCP_client()
{

    // se creaza un socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;             // structura sockaddr_in pentru adresa serverului
    memset(&serv_addr, 0, sizeof(serv_addr)); // se initializeaza cu 0
    serv_addr.sin_family = AF_INET;           // adresa este de tip IPv4
    serv_addr.sin_port = htons(SERVER_PORT);  // portul la care serverul asculta conexiuni

    // tratam cazuri de eroare
    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }
    // tratam cazuri de eroare
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    char message[MESSAGE_LEN];
    while (1)
    {
        printf(">>> ");
        fgets(message, MESSAGE_LEN, stdin); // scriem un mesaj

        // tratam cazuri de eroare
        if (send(sockfd, message, strlen(message), 0) < 0) // se trimite mesajul catre server
        {
            perror("send failed");
            exit(EXIT_FAILURE);
        }

        char response[MESSAGE_LEN];
        int bytes_read = read(sockfd, response, MESSAGE_LEN);

        // tratam cazuri de eroare
        if (bytes_read < 0)
        {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
        response[bytes_read] = '\0';
        printf("::: %s\n", response);
    }
    // se inchide socketul
    close(sockfd);
}

size_t myRead(char *path, char **buffer, size_t size)
{
    int fd;
    size_t bytesRead, totalRead = 0;
    // alocam memorie
    *buffer = (char *)malloc(size);
    // verificam cazurile de eroare
    if (*buffer == NULL)
    {
        return -1;
    }
    // deschidem fisierul
    fd = open(path, O_RDONLY);
    // verificam cazurile de eroare
    if (fd == -1)
    {
        free(*buffer);
        return -1;
    }
    // citim din fisier in bucati
    while ((bytesRead = read(fd, *buffer, BUFSIZE)) > 0)
    {
        totalRead += bytesRead;
    }
    // inchidem fisierul
    close(fd);

    return totalRead;
}

size_t myWrite(char *path, char *buffer, size_t size)
{
    int fd;
    // deschidem fisierul
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    // verificam cazurile de eroare
    if (fd == -1)
    {
        return -1;
    }
    // scriem in fisier in bucati
    ssize_t bytesWritten, totalWritten = 0;
    while ((bytesWritten = write(fd, buffer + totalWritten, BUFSIZE)) < size - totalWritten)
    {
        totalWritten += bytesWritten;
    }
    close(fd);
    return totalWritten;
}

void *inThread(inFile_t *file)
{
    // initilizam datele din structura
    char *filePath = file->filePath;
    size_t size = file->size;
    // folosim functia de citire din fisier
    size_t bytesRead = myRead(filePath, &(file->buffer), size);
    // printam sa vedem date despre fisier
    // fprintf(stdout, "Read %zu bytes from file %s, Buffer: %s\n", bytesRead, filePath, file->buffer);
    fprintf(stdout, "Size:%zu\n", file->size);
    // oprim therad-ul
    pthread_exit((void *)file);
}

void *outThread(outFile_t *file)
{
    // initilizam datele din structura
    char *filePath = file->filePath;
    char *buffer = file->buffer;
    size_t size = file->size;
    // folosim functia de scriere in fisier
    size_t bytesWritten = myWrite(filePath, buffer, size);
    // printam sa vedem date despre fisier
    // fprintf(stdout, "Written %zu bytes to file %s, Buffer: %s\n", bytesWritten, filePath, file->buffer);
    fprintf(stdout, "Size:%zu\n", file->size);
    // oprim therad-ul
    pthread_exit((void *)file);
}

void *fileThread(struct dirent *entry)
{
    struct stat fileStat;
    // citim calea catre fisier/director
    char path[1024];
    sprintf(path, "%s/%s", ".", entry->d_name);
    // verificam de erori
    if (stat(path, &fileStat) < 0)
    {
        printf("Error getting file stats: %s\n", entry->d_name);
        pthread_exit(NULL);
    }
    // afisam date despre fisierul selectat
    printf("Name: %s\n", entry->d_name);
    printf("Size: %ld bytes\n", fileStat.st_size);
    // aici e array-ul in care vedem ce permisiuni are fisierul. daca are o permisiune apare litera, daca nu are o permisiune apare -.
    // e practic un if care verifica daca fisierul are o permisiune
    char permissions[10];
    permissions[0] = (S_ISDIR(fileStat.st_mode)) ? 'd' : '-';
    permissions[1] = (fileStat.st_mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (fileStat.st_mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (fileStat.st_mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (fileStat.st_mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (fileStat.st_mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (fileStat.st_mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (fileStat.st_mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (fileStat.st_mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (fileStat.st_mode & S_IXOTH) ? 'x' : '-';
    // afisam array-ul cu permisiunile
    printf("Permissions: %s\n", permissions);
    // inchidem thread-ul
    pthread_exit(NULL);
}

// am facut functia asta doar ca sa vad dimensiunea fisierului de la input
// practic am luat copy paste din laborator :D
off_t getSizeOfFile(char *path)
{
    int fd_in = open(path, O_RDONLY);
    off_t size = lseek(fd_in, 0L, SEEK_END); // pozitionare de end fisier
    lseek(fd_in, 0L, SEEK_SET);              // pozitionare pe inceput fisier

    return size;
}