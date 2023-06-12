#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#include "utils.h"
#include "client.h"

GtkWidget *grid;
GtkWidget *usernameEntry;
GtkWidget *passwordEntry;
GtkWidget *loginButton;
GtkWidget *statusLabel;
GtkWidget *listView; // New list view widget

char *message;

int getNumberOfUsers()
{
    FILE *file = fopen("credentials.json", "r");
    if (file == NULL)
        return 0;

    char c;
    int count = 0;
    while ((c = fgetc(file)) != EOF)
    {
        if (c == '{')
            count++;
    }

    fclose(file);
    return count;
}

void saveUser(const char username[128], const char email[128], const char password[128])
{
    FILE *file = fopen("credentials.json", "a");
    if (file == NULL)
    {
        printf("Error opening file for writing.\n");
        return;
    }

    int numberOfUsers = getNumberOfUsers();

    if (numberOfUsers > 0)
        fprintf(file, ",");

    fprintf(file, "{\"username\":\"%s\",\"password\":\"%s\",\"email\":\"%s\"}", username, email, password);

    fclose(file);
}

int verifyCredentials(const char username[128], const char password[128])
{
    FILE *file = fopen("credentials.json", "r");
    if (file == NULL)
    {
        printf("Error opening file for reading.\n");
        return 0;
    }

    char line[256];
    char searchString[256];
    sprintf(searchString, "\"username\": \"%s\", \"password\": \"%s\"", username, password);

    while (fgets(line, sizeof(line), file))
    {
        printf("%s\n", searchString);
        printf("%s\n", line);
        if (strstr(line, searchString))
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
}

void addToList(const char *item)
{
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(listView)));

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, item, -1);
}

void selectionChanged(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gchar *item;
        gtk_tree_model_get(model, &iter, 0, &item, -1);
        g_print("Selected File: %s\n", item);

        // // declaram structurile
        // inFile_t inFile;
        // outFile_t outFile;

        // // cerem utilizatorului sa introduca calea catre fisier si cat sa citeasca din el
        // printf("Enter input file path: ");
        // inFile.filePath = malloc(256);
        // strcpy(inFile.filePath, "./drive/");
        // strcat(inFile.filePath, item);
        // printf("%s\n",inFile.filePath);
        // // scanf("%s", inFile.filePath);
        // inFile.size = getSizeOfFile(inFile.filePath);
        // // printf("Enter how much do you want to read: ");
        // // scanf("%zu", &inFile.size);
        // pthread_t thread1;
        // pthread_create(&thread1, NULL, (void *)&inThread, &inFile);

        // // in fisieul de output se scrie ce am citit din fisierul de input
        // printf("Enter output file path: ");
        // outFile.filePath = malloc(256);
        // outFile.filePath = item;
        // printf("%s\n",outFile.filePath);
        // // scanf("%s", outFile.filePath);
        // outFile.buffer = inFile.buffer;
        // outFile.size = inFile.size;
        // // printf("Enter how much do you want to write: ");
        // // scanf("%zu", &outFile.size);
        // pthread_t thread2;
        // pthread_create(&thread2, NULL, (void *)&outThread, &outFile);
        // pthread_join(thread1, NULL);
        // pthread_join(thread2, NULL);

        g_free(item);
    }
}

void createMainApplicationWindow()
{
    GtkWidget *mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Drive Application");
    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 800, 600);

    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);

    listView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Files", renderer, "text", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listView), column);

    gtk_container_add(GTK_CONTAINER(scrolledWindow), listView);
    gtk_container_add(GTK_CONTAINER(mainWindow), scrolledWindow);

    gtk_widget_show_all(mainWindow);
}

void loginButtonClicked(GtkWidget *button, gpointer data)
{
    const char *username = gtk_entry_get_text(GTK_ENTRY(usernameEntry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(passwordEntry));

    GtkWidget *grid = gtk_widget_get_parent(button);

    GtkWidget *statusLabel = gtk_grid_get_child_at(GTK_GRID(grid), 0, 4);

    printf("Username: %s\n", username);
    printf("Password: %s\n", password);

    if (verifyCredentials(username, password))
    {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Login successful!");
        message = username;
        pid_t pid1_A = fork(); // cream primul proces
        if (pid1_A < 0)
        {
            perror("Eroare la crearea procesului A");
            exit(1);
        }
        else if (pid1_A == 0)
        { // copilul
            client(message); 

            // Open your main application window here
            createMainApplicationWindow();

            struct dirent *entry;

            DIR *dir = opendir("./drive");
            if (dir == NULL)
            {
                perror("opendir");
                return;
            }

            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_type == DT_REG)
                { // Check if it's a regular file
                    printf("%s\n", entry->d_name);
                    addToList(entry->d_name);
                }
            }

            closedir(dir);

            // Get the selection object for the list view
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(listView));

            // Connect the selection changed signal to the callback function
            g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(selectionChanged), NULL);
        }
        else{
            wait(NULL);
            exit(0);
        }
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Invalid username or password");
    }
}

void registerButtonClicked(GtkWidget *button, gpointer data)
{
    GtkWidget *emailEntry = (GtkWidget *)g_object_get_data(G_OBJECT(button), "email_entry");
    GtkWidget *passwordEntry = (GtkWidget *)g_object_get_data(G_OBJECT(button), "password_entry");
    GtkWidget *rePasswordEntry = (GtkWidget *)g_object_get_data(G_OBJECT(button), "repassword_entry");
    const char *username = gtk_entry_get_text(GTK_ENTRY(usernameEntry));
    const char *email = gtk_entry_get_text(GTK_ENTRY(emailEntry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(passwordEntry));
    const char *repassword = gtk_entry_get_text(GTK_ENTRY(rePasswordEntry));

    GtkWidget *grid = gtk_widget_get_parent(button);

    GtkWidget *statusLabel = gtk_grid_get_child_at(GTK_GRID(grid), 0, 5);

    if (strcmp(password, repassword) != 0)
    {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Passwords do not match");
        return;
    }

    saveUser(username, email, password);
    char *user_drive = "/drive"; 
    strcpy(user_drive, username);
    mkdir(user_drive, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    gtk_label_set_text(GTK_LABEL(statusLabel), "Registration successful!");
}

void createAccountButtonClicked(GtkWidget *button, gpointer data)
{
    gtk_container_foreach(GTK_CONTAINER(grid), (GtkCallback)gtk_widget_destroy, NULL);

    GtkWidget *usernameLabel = gtk_label_new("Username:");
    gtk_grid_attach(GTK_GRID(grid), usernameLabel, 0, 0, 1, 1);
    usernameEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), usernameEntry, 1, 0, 1, 1);

    GtkWidget *emailLabel = gtk_label_new("Email:");
    gtk_grid_attach(GTK_GRID(grid), emailLabel, 0, 1, 1, 1);
    GtkWidget *emailEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), emailEntry, 1, 1, 1, 1);

    GtkWidget *passwordLabel = gtk_label_new("Password:");
    gtk_grid_attach(GTK_GRID(grid), passwordLabel, 0, 2, 1, 1);
    passwordEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), passwordEntry, 1, 2, 1, 1);
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);

    GtkWidget *rePasswordLabel = gtk_label_new("Re-enter Password:");
    gtk_grid_attach(GTK_GRID(grid), rePasswordLabel, 0, 3, 1, 1);
    GtkWidget *rePasswordEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), rePasswordEntry, 1, 3, 1, 1);
    gtk_entry_set_visibility(GTK_ENTRY(rePasswordEntry), FALSE);

    GtkWidget *registerButton = gtk_button_new_with_label("Register");
    gtk_grid_attach(GTK_GRID(grid), registerButton, 0, 4, 2, 1);
    g_object_set_data(G_OBJECT(registerButton), "email_entry", emailEntry);
    g_object_set_data(G_OBJECT(registerButton), "password_entry", passwordEntry);
    g_object_set_data(G_OBJECT(registerButton), "repassword_entry", rePasswordEntry);
    g_signal_connect(G_OBJECT(registerButton), "clicked", G_CALLBACK(registerButtonClicked), NULL);

    statusLabel = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), statusLabel, 0, 5, 2, 1);

    gtk_widget_show_all(grid);
}

int main(int argc, char *argv[])
{
    GtkWidget *window;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Drive Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 250, 250);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget *usernameLabel = gtk_label_new("Username:");
    gtk_grid_attach(GTK_GRID(grid), usernameLabel, 0, 0, 1, 1);
    usernameEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), usernameEntry, 1, 0, 1, 1);

    GtkWidget *passwordLabel = gtk_label_new("Password:");
    gtk_grid_attach(GTK_GRID(grid), passwordLabel, 0, 1, 1, 1);
    passwordEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), passwordEntry, 1, 1, 1, 1);
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);

    GtkWidget *loginButton = gtk_button_new_with_label("Login");
    gtk_grid_attach(GTK_GRID(grid), loginButton, 0, 2, 2, 1);
    g_signal_connect(G_OBJECT(loginButton), "clicked", G_CALLBACK(loginButtonClicked), NULL);

    GtkWidget *createAccountButton = gtk_button_new_with_label("Create Account");
    gtk_grid_attach(GTK_GRID(grid), createAccountButton, 0, 3, 2, 1);
    g_signal_connect(G_OBJECT(createAccountButton), "clicked", G_CALLBACK(createAccountButtonClicked), NULL);

    statusLabel = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), statusLabel, 0, 4, 2, 1);

    gtk_widget_show_all(window);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    wait(NULL);
    gtk_main();

    return 0;
}