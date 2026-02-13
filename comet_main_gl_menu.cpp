// ============================================================
// COMET BUSTERS - MENU SYSTEM
// ============================================================
// This module handles all menu rendering, navigation, and state
// Separated from main rendering loop for clean architecture
// ============================================================

#ifdef ANDROID
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "cometbuster.h"
#include "comet_lang.h"
#include "comet_main_gl_gui.h"
#include "comet_main_gl_menu.h"

// ============================================================
// FORWARD DECLARATIONS - Graphics functions
// ============================================================
extern void gl_set_color_alpha(float r, float g, float b, float a);
extern void gl_draw_rect_filled(float x, float y, float width, float height);
extern void gl_set_color(float r, float g, float b);
extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);

// ============================================================
// MAIN MENU RENDERING
// ============================================================

static void render_main_menu(CometGUI *gui) {
    gl_set_color(0.0f, 1.0f, 1.0f);
    gl_draw_text_simple("COMET BUSTERS", 800, 100, 28);
    
    const char* const *menu_items = main_menu_items[gui->visualizer.comet_buster.current_language];
    
    int menu_y_start = 300;
    int menu_spacing = 110;
    int menu_width = 400;
    int menu_height = 60;
    int items_per_page = 5;
    int num_menu_items = 10;  // Continue, New Game, High Scores, Save, Load, Audio, Language, Help, Fullscreen, Quit
    
    // Calculate scroll position - keep selected item visible
    int current_selection = gui->menu_selection;
    int scroll_offset = gui->main_menu_scroll_offset;
    
    if (current_selection < scroll_offset) {
        scroll_offset = current_selection;
    }
    
    // Keep selection in view when scrolling down
    if (current_selection >= scroll_offset + items_per_page) {
        scroll_offset = current_selection - items_per_page + 1;
    }
    
    // Clamp scroll offset
    int max_scroll = (num_menu_items > items_per_page) ? 
                    (num_menu_items - items_per_page) : 0;
    if (scroll_offset > max_scroll) {
        scroll_offset = max_scroll;
    }
    
    gui->main_menu_scroll_offset = scroll_offset;
    
    // Draw visible menu items only
    for (int i = 0; i < num_menu_items; i++) {
        // Skip items above scroll area
        if (i < scroll_offset) continue;
        
        // Stop if we've drawn enough for the page
        if (i >= scroll_offset + items_per_page) break;
        
        int display_index = i - scroll_offset;
        int menu_y = menu_y_start + (display_index * menu_spacing);
        int menu_x = (1920 - menu_width) / 2;
        
        if (i == 0 && gui->visualizer.comet_buster.ship_lives <= 0) {
            // Disabled / grayed-out look
            gl_set_color(0.3f, 0.3f, 0.3f);   // dark gray border
            gl_draw_rect_filled(menu_x - 3, menu_y - 3, menu_width + 6, menu_height + 6);

            gl_set_color(0.15f, 0.15f, 0.15f); // inner gray
            gl_draw_rect_filled(menu_x, menu_y, menu_width, menu_height);

            gl_set_color(0.5f, 0.5f, 0.5f);   // lighter gray text color
        } else if (i == current_selection) {
            gl_set_color(1.0f, 1.0f, 0.0f);
            gl_draw_rect_filled(menu_x - 3, menu_y - 3, menu_width + 6, menu_height + 6);
            gl_set_color(0.0f, 0.5f, 1.0f);
            gl_draw_rect_filled(menu_x, menu_y, menu_width, menu_height);
            gl_set_color(1.0f, 1.0f, 0.0f);
        } else {
            gl_set_color(0.2f, 0.2f, 0.4f);
            gl_draw_rect_filled(menu_x, menu_y, menu_width, menu_height);
            gl_set_color(0.7f, 0.7f, 1.0f);
        }
        
        gl_draw_text_simple(menu_items[i], menu_x + 110, menu_y + 25, 24);
    }
    
    // Show scroll indicators if there are more items
    if (num_menu_items > items_per_page) {
        gl_set_color(0.8f, 0.8f, 0.8f);
                        
        char count_text[64];
        sprintf(count_text, "%d / %d", current_selection + 1, num_menu_items);
        gl_set_color(0.7f, 0.7f, 0.9f);
        gl_draw_text_simple(count_text, 880, 920, 12);
    }
    
    gl_set_color(0.8f, 0.8f, 0.8f);
    gl_draw_text_simple(hint_select_close[gui->visualizer.comet_buster.current_language][0], 550, 950, 24);
}

// ============================================================
// DIFFICULTY SELECTION MENU RENDERING
// ============================================================

static void render_difficulty_menu(CometGUI *gui) {
    gl_set_color(0.0f, 1.0f, 1.0f);
    gl_draw_text_simple(label_select_difficulty[gui->visualizer.comet_buster.current_language], 780, 150, 24);
    
    const char* const *difficulties = menu_difficulties[gui->visualizer.comet_buster.current_language];
    
    int diff_y_start = 350;
    int diff_spacing = 120;
    int diff_width = 400;
    int diff_height = 60;
    
    for (int i = 0; i < 3; i++) {
        int diff_y = diff_y_start + (i * diff_spacing);
        int diff_x = (1920 - diff_width) / 2;
        
        if ((i + 1) == gui->gui_difficulty_level) {
            gl_set_color(1.0f, 1.0f, 0.0f);
            gl_draw_rect_filled(diff_x - 3, diff_y - 3, diff_width + 6, diff_height + 6);
            gl_set_color(0.0f, 0.5f, 1.0f);
            gl_draw_rect_filled(diff_x, diff_y, diff_width, diff_height);
            gl_set_color(1.0f, 1.0f, 0.0f);
        } else {
            gl_set_color(0.2f, 0.2f, 0.4f);
            gl_draw_rect_filled(diff_x, diff_y, diff_width, diff_height);
            gl_set_color(0.7f, 0.7f, 1.0f);
        }
        
        gl_draw_text_simple(difficulties[i], diff_x + 140, diff_y + 15, 14);
    }
    
    gl_set_color(0.8f, 0.8f, 0.8f);
    gl_draw_text_simple(hint_continue_back[gui->visualizer.comet_buster.current_language], 550, 950, 10);
}

// ============================================================
// HIGH SCORES MENU RENDERING
// ============================================================

static void render_high_scores_menu(CometGUI *gui) {
    gl_set_color(0.0f, 1.0f, 1.0f);
    gl_draw_text_simple(label_high_scores[gui->visualizer.comet_buster.current_language], 850, 100, 20);
    
    gl_set_color(0.7f, 0.7f, 1.0f);
    CometBusterGame *game = &gui->visualizer.comet_buster;
    int score_y = 250;
    
    // Display high scores from the game
    for (int i = 0; i < 10 && i < game->high_score_count; i++) {
        char score_line[128];
        sprintf(score_line, "%d. %s - %d (Wave %d)", 
                i + 1, 
                game->high_scores[i].player_name,
                game->high_scores[i].score,
                game->high_scores[i].wave);
        gl_draw_text_simple(score_line, 400, score_y + (i * 50), 12);
    }
    
    gl_set_color(0.8f, 0.8f, 0.8f);
    gl_draw_text_simple(hint_continue_back[gui->visualizer.comet_buster.current_language], 700, 950, 10);
}

// ============================================================
// AUDIO MENU RENDERING
// ============================================================

static void render_audio_menu(CometGUI *gui) {
    gl_set_color(0.0f, 1.0f, 1.0f);
    gl_draw_text_simple(label_audio_settings[gui->visualizer.comet_buster.current_language], 820, 150, 18);

    
    const char* const *audio_options = menu_audio_options[gui->visualizer.comet_buster.current_language];
    
    int audio_y_start = 350;
    int audio_spacing = 150;
    
    for (int i = 0; i < 2; i++) {
        int option_y = audio_y_start + (i * audio_spacing);
        int volume = (i == 0) ? gui->music_volume : gui->sfx_volume;
        
        // Title
        if (i == gui->menu_selection) {
            gl_set_color(1.0f, 1.0f, 0.0f);
        } else {
            gl_set_color(0.7f, 0.7f, 1.0f);
        }
        gl_draw_text_simple(audio_options[i], 650, option_y, 16);
        
        // Volume bar background
        gl_set_color(0.2f, 0.2f, 0.4f);
        gl_draw_rect_filled(650, option_y + 40, 600, 30);
        
        // Volume bar fill
        int fill_width = (volume * 600) / 100;
        gl_set_color(0.0f, 1.0f, 0.5f);
        gl_draw_rect_filled(650, option_y + 40, fill_width, 30);
        
        // Volume percentage
        char vol_text[32];
        sprintf(vol_text, "%d%%", volume);
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_text_simple(vol_text, 1300, option_y + 45, 12);
    }
    
    gl_set_color(0.8f, 0.8f, 0.8f);
    gl_draw_text_simple(hint_adjust_menu[gui->visualizer.comet_buster.current_language], 480, 950, 10);
}

// ============================================================
// SAVE STATE MENU RENDERING
// ============================================================

static void render_save_state_menu(CometGUI *gui, int is_load_menu) {
    gl_set_color(0.0f, 1.0f, 1.0f);
    if (is_load_menu) {
        gl_draw_text_simple("LOAD GAME", 850, 100, 24);
    } else {
        gl_draw_text_simple("SAVE GAME", 850, 100, 24);
    }
    
    int state_y_start = 250;
    int state_spacing = 85;
    int state_width = 500;
    int state_height = 60;
    
    for (int i = 0; i < 10; i++) {
        int state_y = state_y_start + (i * state_spacing);
        int state_x = (1920 - state_width) / 2;
        
        if (i == gui->menu_selection) {
            gl_set_color(1.0f, 1.0f, 0.0f);
            gl_draw_rect_filled(state_x - 3, state_y - 3, state_width + 6, state_height + 6);
            gl_set_color(0.0f, 0.5f, 1.0f);
            gl_draw_rect_filled(state_x, state_y, state_width, state_height);
            gl_set_color(1.0f, 1.0f, 0.0f);
        } else {
            gl_set_color(0.2f, 0.2f, 0.4f);
            gl_draw_rect_filled(state_x, state_y, state_width, state_height);
            gl_set_color(0.7f, 0.7f, 1.0f);
        }
        
        char state_label[256];
        const char *info = menu_get_state_info(i);
        snprintf(state_label, sizeof(state_label), "Slot %d: %s", i, info ? info : "Empty");
        gl_draw_text_simple(state_label, state_x + 20, state_y + 20, 14);
    }
    
    gl_set_color(0.8f, 0.8f, 0.8f);
    if (is_load_menu) {
        gl_draw_text_simple("UP/DOWN to select | ENTER to load | ESC to cancel", 350, 950, 12);
    } else {
        gl_draw_text_simple("UP/DOWN to select | ENTER to save | ESC to cancel", 350, 950, 12);
    }
}

// ============================================================
// LANGUAGE MENU RENDERING
// ============================================================

static void render_language_menu(CometGUI *gui) {
    gl_set_color(0.0f, 1.0f, 1.0f);
    gl_draw_text_simple(label_select_language[gui->visualizer.comet_buster.current_language], 800, 150, 20);
    
    int num_languages = sizeof(wlanguagename) / sizeof(wlanguagename[0]);
    int lang_y_start = 300;
    int lang_spacing = 110;
    int lang_width = 400;
    int lang_height = 60;
    int items_per_page = 4;
    
    int current_lang = gui->visualizer.comet_buster.current_language;
    int scroll_offset = gui->lang_menu_scroll_offset;
    
    if (current_lang < scroll_offset) {
        scroll_offset = current_lang;
    }
    
    // Keep selection in view when scrolling down
    if (current_lang >= scroll_offset + items_per_page) {
        scroll_offset = current_lang - items_per_page + 1;
    }
    
    // Clamp scroll offset
    int max_scroll = (num_languages > items_per_page) ? (num_languages - items_per_page) : 0;
    if (scroll_offset > max_scroll) {
        scroll_offset = max_scroll;
    }
    
    gui->lang_menu_scroll_offset = scroll_offset;

    // Draw visible languages only
    for (int i = 0; i < num_languages; i++) {
        // Skip languages above scroll area
        if (i < scroll_offset) continue;
        
        // Stop if we've drawn enough for the page
        if (i >= scroll_offset + items_per_page) break;
        
        int display_index = i - scroll_offset;
        int lang_y = lang_y_start + (display_index * lang_spacing);
        int lang_x = (1920 - lang_width) / 2;
        
        if (i == current_lang) {
            gl_set_color(1.0f, 1.0f, 0.0f);
            gl_draw_rect_filled(lang_x - 3, lang_y - 3, lang_width + 6, lang_height + 6);
            gl_set_color(0.0f, 0.5f, 1.0f);
            gl_draw_rect_filled(lang_x, lang_y, lang_width, lang_height);
            gl_set_color(1.0f, 1.0f, 0.0f);
        } else {
            gl_set_color(0.2f, 0.2f, 0.4f);
            gl_draw_rect_filled(lang_x, lang_y, lang_width, lang_height);
            gl_set_color(0.7f, 0.7f, 1.0f);
        }
        
        gl_draw_text_simple(wlanguagename[i], lang_x + 100, lang_y + 25, 24);
    }
    
    // Show scroll indicators if there are more languages
    if (num_languages > items_per_page) {
        gl_set_color(0.8f, 0.8f, 0.8f);
        
        char count_text[64];
        sprintf(count_text, "%d / %d", current_lang + 1, num_languages);
        gl_set_color(0.7f, 0.7f, 0.9f);
        gl_draw_text_simple(count_text, 880, 920, 12);
    }
    
    gl_set_color(0.8f, 0.8f, 0.8f);
    gl_draw_text_simple(hint_adjust_menu[gui->visualizer.comet_buster.current_language], 480, 950, 10);
}

// ============================================================
// MAIN MENU DISPATCH
// ============================================================
// Renders the appropriate menu based on current menu_state

void render_menu(CometGUI *gui) {
    if (!gui->show_menu) return;
    
    // Semi-transparent overlay
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
    gl_draw_rect_filled(0, 0, 1920, 1080);
    
    // Dispatch to appropriate menu renderer
    switch (gui->menu_state) {
        case 0:
            render_main_menu(gui);
            break;
        case 1:
            render_difficulty_menu(gui);
            break;
        case 2:
            render_high_scores_menu(gui);
            break;
        case 3:
            render_audio_menu(gui);
            break;
        case 4:
            render_language_menu(gui);
            break;
        case 5:
            render_save_state_menu(gui, 0);  // Save menu
            break;
        case 6:
            render_save_state_menu(gui, 1);  // Load menu
            break;
        default:
            break;
    }
}

// ============================================================
// MENU NAVIGATION HELPERS
// ============================================================

/**
 * Updates menu selection for keyboard/joystick navigation
 * Handles wrapping and validation
 */
void menu_move_up(CometGUI *gui) {
    if (gui->menu_state == 0) {
        // Main menu
        int num_menu_items = 10;
        gui->menu_selection = (gui->menu_selection - 1 + num_menu_items) % num_menu_items;
        
        // Update scroll offset
        if (gui->menu_selection < gui->main_menu_scroll_offset) {
            gui->main_menu_scroll_offset = gui->menu_selection;
        }
    } else if (gui->menu_state == 1) {
        // Difficulty menu
        gui->menu_selection = (gui->menu_selection - 1 + 3) % 3;
    } else if (gui->menu_state == 3) {
        // Audio menu
        gui->menu_selection = (gui->menu_selection - 1 + 2) % 2;
    } else if (gui->menu_state == 4) {
        // Language menu
        menu_update_language(gui, -1);
    } else if (gui->menu_state == 5 || gui->menu_state == 6) {
        // Save/Load menu (10 slots)
        gui->menu_selection = (gui->menu_selection - 1 + 10) % 10;
    }
}

/**
 * Updates menu selection downward for keyboard/joystick navigation
 */
void menu_move_down(CometGUI *gui) {
    if (gui->menu_state == 0) {
        // Main menu
        int num_menu_items = 10;
        int items_per_page = 5;
        gui->menu_selection = (gui->menu_selection + 1) % num_menu_items;
        
        // Update scroll offset
        if (gui->menu_selection >= gui->main_menu_scroll_offset + items_per_page) {
            gui->main_menu_scroll_offset = gui->menu_selection - items_per_page + 1;
        }
    } else if (gui->menu_state == 1) {
        // Difficulty menu
        gui->menu_selection = (gui->menu_selection + 1) % 3;
    } else if (gui->menu_state == 3) {
        // Audio menu
        gui->menu_selection = (gui->menu_selection + 1) % 2;
    } else if (gui->menu_state == 4) {
        // Language menu
        menu_update_language(gui, 1);
    } else if (gui->menu_state == 5 || gui->menu_state == 6) {
        // Save/Load menu (10 slots)
        gui->menu_selection = (gui->menu_selection + 1) % 10;
    }
}

/**
 * Updates difficulty selection (left/right navigation in difficulty menu)
 */
void menu_update_difficulty(CometGUI *gui, int direction) {
    gui->gui_difficulty_level += direction;
    if (gui->gui_difficulty_level < 1) gui->gui_difficulty_level = 1;
    if (gui->gui_difficulty_level > 3) gui->gui_difficulty_level = 3;
}

/**
 * Updates audio volume (left/right navigation in audio menu)
 * Called by both keyboard and joystick handlers
 */
void menu_update_volume(CometGUI *gui, int option, int direction) {
    int *volume = (option == 0) ? &gui->music_volume : &gui->sfx_volume;
    int *pref_volume = (option == 0) ? &gui->preferences.music_volume : &gui->preferences.sfx_volume;
    
    *volume += direction * 5;
    if (*volume < 0) *volume = 0;
    if (*volume > 100) *volume = 100;
    
    // Update audio system
    if (option == 0) {
        audio_set_music_volume(&gui->audio, *volume);
    } else {
        audio_set_sfx_volume(&gui->audio, *volume);
    }
    
    // Save preference
    *pref_volume = *volume;
    preferences_save(&gui->preferences);
}

/**
 * Updates language selection with automatic preference save
 * Called by both keyboard and joystick handlers
 */
void menu_update_language(CometGUI *gui, int direction) {
    int num_languages = sizeof(wlanguagename) / sizeof(wlanguagename[0]);
    gui->visualizer.comet_buster.current_language += direction;
    
    if (gui->visualizer.comet_buster.current_language < 0) {
        gui->visualizer.comet_buster.current_language = 0;
    }
    if (gui->visualizer.comet_buster.current_language >= num_languages) {
        gui->visualizer.comet_buster.current_language = num_languages - 1;
    }
    
    // Update scroll offset
    int items_per_page = 4;
    int scroll_offset = gui->lang_menu_scroll_offset;
    
    if (gui->visualizer.comet_buster.current_language < scroll_offset) {
        gui->lang_menu_scroll_offset = gui->visualizer.comet_buster.current_language;
    }
    
    if (gui->visualizer.comet_buster.current_language >= scroll_offset + items_per_page) {
        gui->lang_menu_scroll_offset = gui->visualizer.comet_buster.current_language - items_per_page + 1;
    }
    
    // CRITICAL FIX: Always save language preference (both keyboard and joystick paths)
    gui->preferences.language = gui->visualizer.comet_buster.current_language;
    preferences_save(&gui->preferences);
}

/**
 * Move cheat menu selection up with wrapping
 * Called by both keyboard and joystick handlers
 */
void menu_cheat_move_up(CheatMenuUI *cheat_menu) {
    if (!cheat_menu) return;
    cheat_menu->selection = (cheat_menu->selection - 1 + 7) % 7;
}

/**
 * Move cheat menu selection down with wrapping
 * Called by both keyboard and joystick handlers
 */
void menu_cheat_move_down(CheatMenuUI *cheat_menu) {
    if (!cheat_menu) return;
    cheat_menu->selection = (cheat_menu->selection + 1) % 7;
}

/**
 * Update cheat menu value for selected option
 * Handles wave, lives, missiles, bombs, difficulty with wrapping
 * Called by both keyboard and joystick handlers
 */
void menu_cheat_update_value(CheatMenuUI *cheat_menu, int direction) {
    if (!cheat_menu) return;
    
    switch (cheat_menu->selection) {
        case 0:
            // Wave (1-30)
            cheat_menu->wave += direction;
            if (cheat_menu->wave < 1) cheat_menu->wave = 30;
            if (cheat_menu->wave > 30) cheat_menu->wave = 1;
            break;
        case 1:
            // Lives (1-20)
            cheat_menu->lives += direction;
            if (cheat_menu->lives < 1) cheat_menu->lives = 20;
            if (cheat_menu->lives > 20) cheat_menu->lives = 1;
            break;
        case 2:
            // Missiles (0-99)
            cheat_menu->missiles += direction;
            if (cheat_menu->missiles < 0) cheat_menu->missiles = 99;
            if (cheat_menu->missiles > 99) cheat_menu->missiles = 0;
            break;
        case 3:
            // Bombs (0-99)
            cheat_menu->bombs += direction;
            if (cheat_menu->bombs < 0) cheat_menu->bombs = 99;
            if (cheat_menu->bombs > 99) cheat_menu->bombs = 0;
            break;
        case 4:
            // Difficulty (0-2)
            cheat_menu->cheat_difficulty += direction;
            if (cheat_menu->cheat_difficulty < 0) cheat_menu->cheat_difficulty = 2;
            if (cheat_menu->cheat_difficulty > 2) cheat_menu->cheat_difficulty = 0;
            break;
    }
}

/**
 * Opens a submenu from the main menu
 */
void menu_open_submenu(CometGUI *gui, int submenu_id) {
    gui->menu_state = submenu_id;
    gui->menu_selection = 0;
}

/**
 * Closes current submenu and returns to main menu
 */
void menu_close_submenu(CometGUI *gui) {
    gui->menu_state = 0;
    gui->menu_selection = 0;
}

/**
 * Completely closes the menu system
 */
void menu_close_all(CometGUI *gui) {
    gui->show_menu = false;
    gui->menu_state = 0;
    gui->menu_selection = 0;
}

/**
 * Toggles the menu visibility (pause menu)
 */
void menu_toggle(CometGUI *gui) {
    gui->show_menu = !gui->show_menu;
    if (gui->show_menu) {
        gui->menu_state = 0;
        gui->menu_selection = 0;
    }
}

// ============================================================
// SAVE/LOAD STATE SYSTEM
// ============================================================

#ifdef _WIN32
    #define SAVE_STATE_DIR "AppData\\Local\\CometBusters\\savedata"
#else
    #define SAVE_STATE_DIR "~/.local/share/cometbusters/savedata"
#endif

/**
 * Gets the filename for a save state slot
 */
static void get_state_filename(int slot, char *filename, int max_len) {
    char expanded_dir[512];
    
#ifdef _WIN32
    // Windows: Use %APPDATA%\Local\CometBusters\savedata
    const char *appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s\\Local\\CometBusters\\savedata", appdata);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "CometBusters\\savedata");
    }
#else
    // Linux/Unix: Use ~/.local/share/cometbusters/savedata
    const char *home = getenv("HOME");
    if (home) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s/.local/share/cometbusters/savedata", home);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "./.cometbusters/savedata");
    }
#endif
    
    snprintf(filename, max_len, "%s/comet_state_%d.sav", expanded_dir, slot);
}

/**
 * Ensures the save directory exists
 */
static void ensure_save_dir() {
    char expanded_dir[512];
    
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s\\Local\\CometBusters\\savedata", appdata);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "CometBusters\\savedata");
    }
    
    // Create directory structure on Windows
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir \"%s\" 2>nul", expanded_dir);
    system(cmd);
#else
    const char *home = getenv("HOME");
    if (home) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s/.local/share/cometbusters/savedata", home);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "./.cometbusters/savedata");
    }
    
    // Create directory structure on Unix/Linux
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" 2>/dev/null", expanded_dir);
    system(cmd);
#endif
}

/**
 * Saves the current game state to a slot (0-9)
 */
int menu_save_state(CometGUI *gui, int slot) {
    SDL_Log("[Comet Busters] [SAVE STATE] saving to slot: %i\n", slot);

    if (slot < 0 || slot > 10) return 0;
    
    ensure_save_dir();
    
    char filename[256];
    get_state_filename(slot, filename, sizeof(filename));
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        SDL_Log("[Comet Busters] [SAVE STATE] Failed to open file: %s\n", filename);
        return 0;
    }
    
    // Write header
    int version = 1;
    time_t now = time(NULL);
    
    if (fwrite(&version, sizeof(int), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    if (fwrite(&now, sizeof(time_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    // Save the game structure (CometBusterGame) which contains the actual game data
    CometBusterGame *game = &gui->visualizer.comet_buster;
    if (fwrite(game, sizeof(CometBusterGame), 1, file) != 1) {
        SDL_Log("[Comet Busters] [SAVE STATE] Failed to write game state\n");
        fclose(file);
        return 0;
    }
    
    fclose(file);
    SDL_Log("[Comet Busters] [SAVE STATE] State %d saved to %s (%zu bytes)\n", 
            slot, filename, sizeof(CometBusterGame) + sizeof(int) + sizeof(time_t));
    return 1;
}

/**
 * Loads a game state from a slot (0-9)
 */
int menu_load_state(CometGUI *gui, int slot) {
    SDL_Log("[Comet Busters] [Load STATE] saving to slot: %i\n", slot);

    if (slot < 0 || slot > 10) return 0;
    
    char filename[256];
    get_state_filename(slot, filename, sizeof(filename));
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        SDL_Log("[Comet Busters] [LOAD STATE] Failed to open file: %s\n", filename);
        return 0;
    }
    
    // Read and verify header
    int version;
    time_t timestamp;
    
    if (fread(&version, sizeof(int), 1, file) != 1) {
        SDL_Log("[Comet Busters] [LOAD STATE] Failed to read version\n");
        fclose(file);
        return 0;
    }
    
    if (version != 1) {
        SDL_Log("[Comet Busters] [LOAD STATE] Unsupported save version: %d\n", version);
        fclose(file);
        return 0;
    }
    
    if (fread(&timestamp, sizeof(time_t), 1, file) != 1) {
        SDL_Log("[Comet Busters] [LOAD STATE] Failed to read timestamp\n");
        fclose(file);
        return 0;
    }
    
    // Load the game structure
    CometBusterGame *game = &gui->visualizer.comet_buster;

    int saved_language = game->current_language;

    if (fread(game, sizeof(CometBusterGame), 1, file) != 1) {
        SDL_Log("[Comet Busters] [LOAD STATE] Failed to read game state\n");
        fclose(file);
        return 0;
    }
    game->current_language = saved_language;
    fclose(file);
    SDL_Log("[Comet Busters] [LOAD STATE] State %d loaded from %s (%zu bytes)\n", 
            slot, filename, sizeof(CometBusterGame) + sizeof(int) + sizeof(time_t));
    return 1;
}

/**
 * Checks if a save state exists
 */
int menu_state_exists(int slot) {
    if (slot < 0 || slot > 9) return 0;
    
    char filename[256];
    get_state_filename(slot, filename, sizeof(filename));
    
    FILE *file = fopen(filename, "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

/**
 * Gets human-readable info about a save state
 */
static char state_info_buffer[256];

const char* menu_get_state_info(int slot) {
    if (slot < 0 || slot > 9) {
        return "Invalid";
    }
    
    if (!menu_state_exists(slot)) {
        return "Empty";
    }
    
    char filename[256];
    get_state_filename(slot, filename, sizeof(filename));
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return "Error";
    }
    
    int version;
    time_t timestamp;
    CometBusterGame game;
    
    // Read version
    if (fread(&version, sizeof(int), 1, file) != 1) {
        fclose(file);
        return "Error";
    }
    
    // Read timestamp
    if (fread(&timestamp, sizeof(time_t), 1, file) != 1) {
        fclose(file);
        return "Error";
    }
    
    // Read game data
    if (fread(&game, sizeof(CometBusterGame), 1, file) != 1) {
        fclose(file);
        return "Error";
    }
    
    fclose(file);
    
    // Format: "Wave X Score Y HH:MM"
    struct tm *timeinfo = localtime(&timestamp);
    snprintf(state_info_buffer, sizeof(state_info_buffer), 
             "Wave %d | Score %d | %02d:%02d",
             game.current_wave, game.score, timeinfo->tm_hour, timeinfo->tm_min);
    
    return state_info_buffer;
}
