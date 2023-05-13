#include <gtk/gtk.h>

int main() {
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *button;

  gtk_init();

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Drive Application");
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);


  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(window), grid);


  label = gtk_label_new("Welcome to Drive Application!");
  gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

  button = gtk_button_new_with_label("Login to Drive");/
  gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 1, 1);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}