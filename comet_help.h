#ifndef COMET_HELP_H
#define COMET_HELP_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Data structure for passing game state to help dialog callback
 */
typedef struct {
    GtkWidget *window;
    bool *game_paused;
} CometHelpUserData;

/**
 * Display the CometBuster About/Help dialog with multiple information tabs
 */
void on_menu_about_comet(GtkMenuItem *menuitem, gpointer user_data);

#endif // COMET_HELP_H
