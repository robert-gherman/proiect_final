d;

    while (1) {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // eliminăm caracterul newline

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Eroare la fork()\n");
            exit(1);
        } else if (pid == 0) {
            // copil
            execlp("/bin/sh", "/bin/sh", "-c", buffer, NULL);
            fprintf(stderr, "Eroare la exec()\n");
            exit(1);
        } else {
            // părinte
            wait(NULL);
        }
    }

    return 0;
}
//FOOTER
//comanda rulare "./app"
//output-ul primit :
// "> ls -l
// total 24
// -rwxrwxr-x 1 robi robi 16432 mar 26 18:38 app
// -rw-rw-r-- 1 robi robi  1007 mar 26 18:37 app.c
// > exit
// "