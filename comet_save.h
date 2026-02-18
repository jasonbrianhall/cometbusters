#ifndef COMET_SAVE_H
#define COMET_SAVE_H

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

#endif
