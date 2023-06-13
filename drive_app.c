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

typedef struct FileData {
    char name[256];
    char owner[256];
    char dateCreated[256];
    char size[256];
} FileData;

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
    return 0;
}

void addToList(const char *item, const char *owner, const char *dateCreated, const char *size) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(listView)));

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, item, 1, owner, 2, dateCreated, 3, size, -1);
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

gchar *selectedFileName = NULL;
// Callback function for the copy menu item
void copyFileCallback(GtkWidget *widget, gpointer data)
{
    // Create the source and destination file paths
    gchar *srcPath = g_strdup_printf("./drive/%s", selectedFileName);
    gchar *dstPath = g_strdup_printf("./copied_drive/%s", selectedFileName);

    FILE *srcFile = fopen(srcPath, "rb");
    if (srcFile == NULL)
    {
        g_print("Error opening source file for reading.\n");
        g_free(srcPath);
        g_free(dstPath);
        return;
    }

    FILE *dstFile = fopen(dstPath, "wb");
    if (dstFile == NULL)
    {
        g_print("Error opening destination file for writing.\n");
        fclose(srcFile);
        g_free(srcPath);
        g_free(dstPath);
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0)
    {
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, dstFile);
        if (bytesWritten < bytesRead)
        {
            g_print("Error writing to destination file.\n");
            fclose(srcFile);
            fclose(dstFile);
            g_free(srcPath);
            g_free(dstPath);
            return;
        }
    }

    fclose(srcFile);
    fclose(dstFile);
    g_print("File '%s' copied to '%s'\n", srcPath, dstPath);
    g_free(srcPath);
    g_free(dstPath);
}

// Callback function for the delete menu item
void deleteFileCallback(GtkWidget *widget, gpointer data)
{
    // Check if a file is selected
    if (selectedFileName == NULL) {
        printf("No file selected.\n");
        return;
    }

    // Create the file path
    char* filePath = g_strdup_printf("./drive/%s", selectedFileName);

    // Delete the selected file
    if (remove(filePath) == 0)
        printf("File deleted successfully.\n");
    else
        printf("Failed to delete the file.\n");

    g_free(filePath);
}
// Create the popup menu
void createPopupMenu()
{
    popupMenu = gtk_menu_new();

    // Copy menu item
    GtkWidget *copyMenuItem = gtk_menu_item_new_with_label("Copy");
    g_signal_connect(G_OBJECT(copyMenuItem), "activate", G_CALLBACK(copyFileCallback), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), copyMenuItem);

    // Delete menu item
    GtkWidget *deleteMenuItem = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(G_OBJECT(deleteMenuItem), "activate", G_CALLBACK(deleteFileCallback), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), deleteMenuItem);

    gtk_widget_show_all(popupMenu);
}

// Callback function for the popup menu
gboolean popupMenuCallback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    if (event->type == GDK_BUTTON_PRESS) {
        GdkEventButton *buttonEvent = (GdkEventButton *)event;
        if (buttonEvent->button == GDK_BUTTON_SECONDARY) {
            // Show the popup menu at the pointer position
            gtk_menu_popup(GTK_MENU(popupMenu), NULL, NULL, NULL, NULL, buttonEvent->button, buttonEvent->time);
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
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
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
        pid_t pid1_A = fork(); // Create the first process
        if (pid1_A < 0)
        {
            perror("Error creating process A");
            exit(1);
        }
        else if (pid1_A == 0)
        { // Child process
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
                    char filePath[256];
                    snprintf(filePath, sizeof(filePath), "./drive/%s", entry->d_name);

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

                        addToList(entry->d_name, username, dateCreated, size);
                    }
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
     char user_drive[256];  // Allocate memory for user_drive
    strcpy(user_drive, "./drive/");
    strcat(user_drive, username);  // Concatenate the username to user_drive

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