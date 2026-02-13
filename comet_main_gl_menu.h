// ============================================================
// COMET BUSTERS - MENU SYSTEM HEADER
// ============================================================
// Public interface for the menu system module
// ============================================================

#ifndef COMET_MAIN_GL_MENU_H
#define COMET_MAIN_GL_MENU_H

#include "comet_main_gl_gui.h"

// ============================================================
// MENU RENDERING
// ============================================================

/**
 * Renders the current menu state
 * Handles overlay, background, and dispatches to menu-specific renderers
 */
void render_menu(CometGUI *gui);

// ============================================================
// MENU NAVIGATION
// ============================================================

/**
 * Moves selection up in the current menu
 * Handles wrapping for all menu types
 */
void menu_move_up(CometGUI *gui);

/**
 * Moves selection down in the current menu
 * Handles wrapping and scroll offset updates
 */
void menu_move_down(CometGUI *gui);

/**
 * Updates difficulty selection during difficulty menu
 * @param direction: -1 for left, +1 for right
 */
void menu_update_difficulty(CometGUI *gui, int direction);

/**
 * Updates audio volume during audio menu
 * @param option: 0 for music, 1 for SFX
 * @param direction: -1 for decrease, +1 for increase
 */
void menu_update_volume(CometGUI *gui, int option, int direction);

/**
 * Updates language selection during language menu
 * @param direction: -1 for previous, +1 for next
 */
void menu_update_language(CometGUI *gui, int direction);

// ============================================================
// MENU STATE MANAGEMENT
// ============================================================

/**
 * Opens a submenu from the main menu
 * @param submenu_id: 0=Main, 1=Difficulty, 2=HighScores, 3=Audio, 4=Language
 */
void menu_open_submenu(CometGUI *gui, int submenu_id);

/**
 * Closes current submenu and returns to main menu
 */
void menu_close_submenu(CometGUI *gui);

/**
 * Completely closes the menu system
 */
void menu_close_all(CometGUI *gui);

/**
 * Toggles the menu visibility (pause menu)
 */
void menu_toggle(CometGUI *gui);

#endif // COMET_MAIN_GL_MENU_H
