#include <gtk/gtk.h>
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

// #include "INETclient.h"
#include "UNIXclient.h"

GtkWidget *grid;
GtkWidget *usernameEntry;
GtkWidget *passwordEntry;
GtkWidget *loginButton;
GtkWidget *statusLabel;
GtkWidget *listView;  // New list view widget
GtkWidget *listView1; // New list view widget

int sock;
char *uname;
char *message;

typedef struct FileData
{
    char name[256];
    char owner[256];
    char dateCreated[256];
    char size[256];
} FileData;

void addFileToList(const char *item, const char *owner, const char *dateCreated, const char *size)
{
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(listView)));

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, item, 1, owner, 2, dateCreated, 3, size, -1);
}

void addToList(const char *item1)
{
    GtkListStore *store1 = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(listView1)));

    GtkTreeIter iter1;
    gtk_list_store_append(store1, &iter1);
    gtk_list_store_set(store1, &iter1, 0, item1, -1);
}

void printUsers(const char *filename)
{
    // Load the JSON file
    json_t *root;
    json_error_t error;
    root = json_load_file(filename, 0, &error);

    if (!root)
    {
        fprintf(stderr, "Error loading JSON file: %s\n", error.text);
        return;
    }

    // Ensure the root is an array
    if (!json_is_array(root))
    {
        fprintf(stderr, "Error: JSON file does not contain an array.\n");
        json_decref(root);
        return;
    }

    // Iterate through the JSON array and print the users
    size_t arraySize = json_array_size(root);
    printf("List of users:\n");
    for (size_t i = 0; i < arraySize; i++)
    {
        json_t *jsonObject = json_array_get(root, i);
        json_t *usernameValue = json_object_get(jsonObject, "username");

        if (json_is_string(usernameValue))
        {
            const char *username = json_string_value(usernameValue);
            addToList(username);
            printf("Username: %s\n", username);
        }
    }

    // Cleanup
    json_decref(root);
}

int copyFile(const char *srcPath, const char *dstPath)
{
    FILE *srcFile = fopen(srcPath, "rb");
    if (srcFile == NULL)
    {
        printf("Error opening source file for reading.\n");
        return 0;
    }

    FILE *dstFile = fopen(dstPath, "wb");
    if (dstFile == NULL)
    {
        printf("Error opening destination file for writing.\n");
        fclose(srcFile);
        return 0;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0)
    {
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, dstFile);
        if (bytesWritten < bytesRead)
        {
            printf("Error writing to destination file.\n");
            fclose(srcFile);
            fclose(dstFile);
            return 0;
        }
    }

    fclose(srcFile);
    fclose(dstFile);
    return 1;
}

GtkWidget *listView;
GtkWidget *popupMenu;
GtkWidget *popupMenu1;

gchar *selectedFileName = NULL;
// Callback function for the copy menu item
void copyFileCallback(GtkWidget *widget, gpointer data)
{
    char message[1024];

    snprintf(message, sizeof(message), "download:%s:%s", uname, selectedFileName);
    send_message(sock, message);
    receive_file(sock, uname, selectedFileName);
}
// Callback function for the delete menu item
void deleteFileCallback(GtkWidget *widget, gpointer data)
{
    // Check if a file is selected
    if (selectedFileName == NULL)
    {
        printf("No file selected.\n");
        return;
    }

    // Create the file path
    char *filePath = g_strdup_printf("./drive/%s/%s", uname, selectedFileName);

    // Delete the selected file
    if (remove(filePath) == 0)
        printf("File deleted successfully.\n");
    else
        printf("Failed to delete the file.\n");

    g_free(filePath);
}
// Create the regular user popup menu
void createPopupMenu()
{
    popupMenu = gtk_menu_new();

    // Download menu item
    GtkWidget *downloadMenuItem = gtk_menu_item_new_with_label("Download");
    g_signal_connect(G_OBJECT(downloadMenuItem), "activate", G_CALLBACK(copyFileCallback), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), downloadMenuItem);

    // Delete menu item
    GtkWidget *deleteMenuItem = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(G_OBJECT(deleteMenuItem), "activate", G_CALLBACK(deleteFileCallback), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), deleteMenuItem);

    gtk_widget_show_all(popupMenu);
}

GtkWidget *popupMenu;      // Popup menu for the regular user
GtkWidget *adminPopupMenu; // Popup menu for the admin user

void deleteUserCallback() {}
void disconnectCallback() {}
void blockUserCallback() {}
void stopProcessCallback() {}

// Create the admin user popup menu
void createAdminPopupMenu()
{
    adminPopupMenu = gtk_menu_new();

    // Delete menu item
    GtkWidget *deleteMenuItem = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(G_OBJECT(deleteMenuItem), "activate", G_CALLBACK(deleteUserCallback), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(adminPopupMenu), deleteMenuItem);

    gtk_widget_show_all(adminPopupMenu);
}


// Callback function for the popup menu
gboolean popupMenuCallback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        GdkEventButton *buttonEvent = (GdkEventButton *)event;
        if (buttonEvent->button == GDK_BUTTON_SECONDARY)
        {
            // Show the popup menu at the pointer position
            gtk_menu_popup(GTK_MENU(popupMenu), NULL, NULL, NULL, NULL, buttonEvent->button, buttonEvent->time);
            return TRUE;
        }
    }
    return FALSE;
}

// Callback function for the popup menu
gboolean popupAdminMenuCallback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        GdkEventButton *buttonEvent = (GdkEventButton *)event;
        if (buttonEvent->button == GDK_BUTTON_SECONDARY)
        {
            // Show the popup menu at the pointer position
            gtk_menu_popup(GTK_MENU(adminPopupMenu), NULL, NULL, NULL, NULL, buttonEvent->button, buttonEvent->time);
            return TRUE;
        }
    }
    return FALSE;
}


// Callback function for the selection changed event
void selectionChanged(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gchar *item;
        gtk_tree_model_get(model, &iter, 0, &item, -1);
        g_print("Selected File: %s\n", item);

        // Store the selected file name
        if (selectedFileName != NULL)
            g_free(selectedFileName);
        selectedFileName = g_strdup(item);

        g_free(item);
    }
}
// Create the main application window
void createMainApplicationWindow()
{
    GtkWidget *mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Drive Application");
    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 800, 600);

    GtkListStore *store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    listView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Files",
                                                                         renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listView), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 200); // Set a fixed width for the column

    column = gtk_tree_view_column_new_with_attributes("Owner",
                                                      renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listView), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 200); // Set a fixed width for the column

    column = gtk_tree_view_column_new_with_attributes("Date created",
                                                      renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listView), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 200); // Set a fixed width for the column

    column = gtk_tree_view_column_new_with_attributes("Size",
                                                      renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listView), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 200); // Set a fixed width for the column

    // Create the tree selection and set the selection changed callback
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(listView));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(selectionChanged), NULL);

    // Set the right-click popup menu callback
    g_signal_connect(G_OBJECT(listView), "button-press-event", G_CALLBACK(popupMenuCallback), NULL);

    // Add the tree view to a scrolled window
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), listView);

    // Add the scrolled window to the main window
    gtk_container_add(GTK_CONTAINER(mainWindow), scrolledWindow);

    // Show all the widgets
    gtk_widget_show_all(mainWindow);

    // Create the popup menu
    createPopupMenu();
}

void createAdminApplicationWindow()
{
    GtkWidget *adminWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(adminWindow), "Admin Window");
    gtk_window_set_default_size(GTK_WINDOW(adminWindow), 400, 300);
    g_signal_connect(adminWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING); // Only 1 column is needed

    listView1 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Files", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listView1), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 200); // Set a fixed width for the column

    // Add the tree view to a scrolled window
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), listView1);

    // Add the scrolled window to the main window
    gtk_container_add(GTK_CONTAINER(adminWindow), scrolledWindow);

    // Create the tree selection and set the selection changed callback
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(listView1));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(selectionChanged), NULL);

    // Set the right-click popup menu callback
    g_signal_connect(G_OBJECT(listView1), "button-press-event", G_CALLBACK(popupAdminMenuCallback), NULL);

    printUsers("credentials.json");

    gtk_widget_show_all(adminWindow);

    createAdminPopupMenu();
}

void loginButtonClicked(GtkWidget *button, gpointer data)
{
    const char *username = gtk_entry_get_text(GTK_ENTRY(usernameEntry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(passwordEntry));

    GtkWidget *grid = gtk_widget_get_parent(button);

    GtkWidget *statusLabel = gtk_grid_get_child_at(GTK_GRID(grid), 0, 4);

    char message[1024];
    snprintf(message, sizeof(message), "login:username:%s:password:%s", username, password);
    send_message(sock, message);
    const char *response;
    response = receive_message(sock);
    printf("%s\n", response);
    if (strcmp(response, "OK") == 0)
    {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Login successful!");
        pid_t pid1_A = fork(); // cream primul proces
        if (pid1_A < 0)
        {
            perror("Eroare la crearea procesului A");
            exit(1);
        }
        else if (pid1_A == 0)
        { // copilul
            uname = username;
            // Open your main application window here
            createMainApplicationWindow();

            char drive[256];
            snprintf(drive, sizeof(drive), "./drive/%s", username);
            printf("%s\n", drive);
            DIR *dir = opendir(drive);
            if (dir == NULL)
            {
                perror("opendir");
                return;
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_type == DT_REG)
                { // Check if it's a regular file
                    char filePath[256];
                    printf("%s\n", entry->d_name);
                    snprintf(filePath, sizeof(filePath), "./%s/%s", drive, entry->d_name);
                    struct stat fileStat;
                    if (stat(filePath, &fileStat) == 0)
                    {

                        char dateCreated[256] = "Unknown";
                        char size[256] = "Unknown";

                        // Extract owner information
                        struct passwd *pw = getpwuid(fileStat.st_uid);

                        // Extract date created information
                        struct tm *t = localtime(&fileStat.st_ctime);
                        strftime(dateCreated, sizeof(dateCreated), "%Y-%m-%d %H:%M:%S", t);

                        // Extract size information
                        snprintf(size, sizeof(size), "%lld bytes", (long long)fileStat.st_size);

                        addFileToList(entry->d_name, username, dateCreated, size);
                    }
                }
            }

            closedir(dir);
        }
        else
        {
            wait(NULL);
            exit(0);
        }
    }
    if (strcmp(response, "NO") == 0)
    {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Invalid username or password");
        free(response);
        response = NULL;
    }

    if (strcmp(response, "ADMIN") == 0)
    {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Login successful!");
        pid_t pid1_A = fork(); // cream primul proces
        if (pid1_A < 0)
        {
            perror("Eroare la crearea procesului A");
            exit(1);
        }
        else if (pid1_A == 0)
        { // copilul
            uname = username;
            // Open your main application window here
            createAdminApplicationWindow();
            createMainApplicationWindow();

            char drive[256];
            snprintf(drive, sizeof(drive), "./drive/%s", username);
            printf("%s\n", drive);
            DIR *dir = opendir(drive);
            if (dir == NULL)
            {
                perror("opendir");
                return;
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_type == DT_REG)
                { // Check if it's a regular file
                    char filePath[256];
                    printf("%s\n", entry->d_name);
                    snprintf(filePath, sizeof(filePath), "./%s/%s", drive, entry->d_name);
                    struct stat fileStat;
                    if (stat(filePath, &fileStat) == 0)
                    {

                        char dateCreated[256] = "Unknown";
                        char size[256] = "Unknown";

                        // Extract owner information
                        struct passwd *pw = getpwuid(fileStat.st_uid);

                        // Extract date created information
                        struct tm *t = localtime(&fileStat.st_ctime);
                        strftime(dateCreated, sizeof(dateCreated), "%Y-%m-%d %H:%M:%S", t);

                        // Extract size information
                        snprintf(size, sizeof(size), "%lld bytes", (long long)fileStat.st_size);

                        addFileToList(entry->d_name, username, dateCreated, size);
                    }
                }
            }

            closedir(dir);
        }
        else
        {
            wait(NULL);
            exit(0);
        }
    }

    free(response);
    response = NULL;
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

    char message[1024];
    snprintf(message, sizeof(message), "register:username:%s:email:%s:password:%s", username, email, password);
    send_message(sock, message);
    receive_message(sock);

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

    sock = connect_to_server();
    if (sock == -1)
    {
        printf("Failed to connect to the server\n");
        exit(EXIT_FAILURE);
    }

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