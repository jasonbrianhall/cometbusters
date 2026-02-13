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
 * Updates audio volume during audio menu with preference save
 * @param gui: GUI state
 * @param option: 0 for music, 1 for SFX
 * @param direction: -1 for decrease, +1 for increase
 */
void menu_update_volume(CometGUI *gui, int option, int direction);

/**
 * Updates language selection during language menu with preference save
 * @param direction: -1 for previous, +1 for next
 */
void menu_update_language(CometGUI *gui, int direction);

/**
 * Updates cheat menu selection up with wrapping
 */
void menu_cheat_move_up(CheatMenuUI *cheat_menu);

/**
 * Updates cheat menu selection down with wrapping
 */
void menu_cheat_move_down(CheatMenuUI *cheat_menu);

/**
 * Updates cheat menu value (wave, lives, missiles, bombs, difficulty)
 * @param cheat_menu: Cheat menu state
 * @param direction: -1 for decrease, +1 for increase
 */
void menu_cheat_update_value(CheatMenuUI *cheat_menu, int direction);

// ============================================================
// SAVE/LOAD STATE MANAGEMENT
// ============================================================

/**
 * Saves the current game state to a slot (0-9)
 * Returns 1 on success, 0 on failure
 */
int menu_save_state(CometGUI *gui, int slot);

/**
 * Loads a game state from a slot (0-9)
 * Returns 1 on success, 0 on failure
 */
int menu_load_state(CometGUI *gui, int slot);

/**
 * Checks if a save state exists
 * Returns 1 if exists, 0 if not
 */
int menu_state_exists(int slot);

/**
 * Gets human-readable info about a save state
 * Returns timestamp string
 */
const char* menu_get_state_info(int slot);

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
