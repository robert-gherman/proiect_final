#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GtkWidget *grid;
GtkWidget *usernameEntry;
GtkWidget *passwordEntry;
GtkWidget *loginButton;
GtkWidget *statusLabel;

int checkCredentials(const char *username, const char *password) {
    char buffer[100];
    FILE *file = fopen("credentials.txt", "r");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 0;
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        char storedUsername[50];
        char storedPassword[50];
        sscanf(buffer, "%s %s", storedUsername, storedPassword);
        if (strcmp(username, storedUsername) == 0 && strcmp(password, storedPassword) == 0) {
            fclose(file);
            return 1;
        }
    }
    
    fclose(file);
    return 0;
}

void loginButtonClicked(GtkWidget *button, gpointer data) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(usernameEntry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(passwordEntry));

    GtkWidget *grid = gtk_widget_get_parent(button);

    GtkWidget *statusLabel = gtk_grid_get_child_at(GTK_GRID(grid), 0, 4); 

    printf("Username: %s\n", username);
    printf("Password: %s\n", password);

    if (checkCredentials(username, password)) {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Login successful!");
    } else {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Invalid username or password");
    }
}
void registerButtonClicked(GtkWidget *button, gpointer data) {
    GtkWidget *emailEntry = (GtkWidget *)g_object_get_data(G_OBJECT(button), "email_entry");
    GtkWidget *passwordEntry = (GtkWidget *)g_object_get_data(G_OBJECT(button), "password_entry");
    GtkWidget *rePasswordEntry = (GtkWidget *)g_object_get_data(G_OBJECT(button), "repassword_entry");
    const char *username = gtk_entry_get_text(GTK_ENTRY(usernameEntry));
    const char *email = gtk_entry_get_text(GTK_ENTRY(emailEntry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(passwordEntry));
    const char *repassword = gtk_entry_get_text(GTK_ENTRY(rePasswordEntry));

    GtkWidget *grid = gtk_widget_get_parent(button);

    GtkWidget *statusLabel = gtk_grid_get_child_at(GTK_GRID(grid), 0, 5); 

    if (strcmp(password, repassword) != 0) {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Passwords do not match");
        return;
    }

    FILE *file = fopen("credentials.txt", "a");
    if (file == NULL) {
        printf("Error opening file!\n");
        gtk_label_set_text(GTK_LABEL(statusLabel), "Registration failed");
        return;
    }

    fprintf(file, "%s %s %s %s\n", username, email, password, repassword);
    fclose(file);

    gtk_label_set_text(GTK_LABEL(statusLabel), "Registration successful!");
}

void createAccountButtonClicked(GtkWidget *button, gpointer data) {
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

int main(int argc, char *argv[]) {
    GtkWidget *window;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Drive Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 600);
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

    gtk_main();

    return 0;
}

   

