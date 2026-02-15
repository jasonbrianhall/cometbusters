#ifdef ANDROID
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "cometbuster.h"
#include "visualization.h"
#include "audio_wad.h"
#include "comet_lang.h"
#include "comet_preferences.h"
#include "comet_main_gl_gui.h"
#include "comet_main_gl_menu.h"

// ============================================================
// QUICK SAVE/LOAD HELPER FUNCTIONS (for F5/F7 - slot 10)
// ============================================================

static void get_quicksave_filename(char *filename, size_t max_len) {
    char expanded_dir[512];
    
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s\\Local\\CometBusters\\savedata", appdata);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "CometBusters\\savedata");
    }
#else
    const char *home = getenv("HOME");
    if (home) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s/.local/share/cometbusters/savedata", home);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "./.cometbusters/savedata");
    }
#endif
    
    snprintf(filename, max_len, "%s/comet_state_10.sav", expanded_dir);
}

static void ensure_quicksave_dir() {
    char expanded_dir[512];
    
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(expanded_dir, sizeof(expanded_dir), "%s\\Local\\CometBusters\\savedata", appdata);
    } else {
        snprintf(expanded_dir, sizeof(expanded_dir), "CometBusters\\savedata");
    }
    
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
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" 2>/dev/null", expanded_dir);
    system(cmd);
#endif
}

// ============================================================

void handle_events(CometGUI *gui, HighScoreEntryUI *hs_entry, CheatMenuUI *cheat_menu) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                gui->running = false;
                break;
            
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || 
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GetWindowSize(gui->window, &gui->window_width, &gui->window_height);
                    SDL_Log("[Comet Busters] [WINDOW] Physical window resized to %dx%d\n", gui->window_width, gui->window_height);
                    
                    // On Android, enforce fixed rendering resolution for performance
#ifdef ANDROID
                    // Always render at 720x480 on Android, regardless of device resolution
                    gui->visualizer.width = 720;
                    gui->visualizer.height = 480;
                    glViewport(0, 0, 720, 480);
                    SDL_Log("[Comet Busters] [ANDROID] Fixed render resolution at 720x480 (physical window: %dx%d)\n",
                            gui->window_width, gui->window_height);
#else
                    // Desktop: render at window resolution
                    gui->visualizer.width = gui->window_width;
                    gui->visualizer.height = gui->window_height;
                    glViewport(0, 0, gui->window_width, gui->window_height);
#endif
                }
                break;
            
            case SDL_KEYDOWN: {
                // Check if we're in high score entry mode
                if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE) {
                    handle_keyboard_input(&event, gui, hs_entry);
                    break;  // Skip other input handling while entering name
                }
                
                // Check for splash screen exit on any key
                if (gui->visualizer.comet_buster.splash_screen_active) {
                    SDL_Log("[Comet Busters] [SPLASH] User pressed key - exiting splash screen\n");
                    
                    // Stop the intro music
                    audio_stop_music(&gui->audio);
                    
                    // Exit the splash screen
                    gui->visualizer.comet_buster.splash_screen_active = false;
                    
                    // Clear the board completely
                    gui->visualizer.comet_buster.comet_count = 0;
                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                    gui->visualizer.comet_buster.bullet_count = 0;
                    gui->visualizer.comet_buster.particle_count = 0;
                    gui->visualizer.comet_buster.floating_text_count = 0;
                    gui->visualizer.comet_buster.canister_count = 0;
                    gui->visualizer.comet_buster.missile_count = 0;
                    gui->visualizer.comet_buster.missile_pickup_count = 0;
                    gui->visualizer.comet_buster.bomb_count = 0;
                    gui->visualizer.comet_buster.bomb_pickup_count = 0;
                    gui->visualizer.comet_buster.score = 0;
                    gui->visualizer.comet_buster.score_multiplier = 1.0;
                    gui->visualizer.comet_buster.game_over = false;
                    gui->visualizer.comet_buster.game_won = false;
                    
                    // Spawn wave 1
#ifndef ANDROID
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    audio_play_random_music(&gui->audio);
                    SDL_Log("[Comet Busters] [SPLASH] Started gameplay music\n");
#endif
                    break;  // Skip other input processing when exiting splash
                }
                
                // Handle escape for menu toggle
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // Close help overlay and return to main menu
                    if (gui->show_help_overlay) {
                        gui->show_help_overlay = false;
                        gui->help_scroll_offset = 0;
                        gui->menu_state = 0;
                        gui->menu_selection = 5;  // Keep Help selected
                    } else if (gui->show_menu && gui->menu_state != 0) {
                        menu_close_submenu(gui);
                    gui->menu_selection = 1;
                    } else {
                        gui->show_menu = !gui->show_menu;
                        menu_open_submenu(gui, 0);
                        if (gui->visualizer.comet_buster.ship_lives<=0) {
                            comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, MEDIUM);
                        }
                    }
                    break;
                }
                // Handle cheat menu and menu input
                if (gui->show_menu) {
                    // Handle cheat menu input first (takes priority)
                    if (cheat_menu && cheat_menu->state == CHEAT_MENU_OPEN) {
                        if (event.key.keysym.sym == SDLK_UP) {
                    menu_cheat_move_up(cheat_menu);
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    menu_cheat_move_down(cheat_menu);
                        } else if (event.key.keysym.sym == SDLK_LEFT) {
                            switch (cheat_menu->selection) {
                                case 0: cheat_menu->wave = (cheat_menu->wave - 1 < 1) ? 30 : cheat_menu->wave - 1; break;
                                case 1: cheat_menu->lives = (cheat_menu->lives - 1 < 1) ? 20 : cheat_menu->lives - 1; break;
                                case 2: cheat_menu->missiles = (cheat_menu->missiles - 1 < 0) ? 99 : cheat_menu->missiles - 1; break;
                                case 3: cheat_menu->bombs = (cheat_menu->bombs - 1 < 0) ? 99 : cheat_menu->bombs - 1; break;
                                case 4: cheat_menu->cheat_difficulty = (cheat_menu->cheat_difficulty - 1 < 0) ? 2 : cheat_menu->cheat_difficulty - 1; break;
                            }
                        } else if (event.key.keysym.sym == SDLK_RIGHT) {
                            switch (cheat_menu->selection) {
                                case 0: cheat_menu->wave = (cheat_menu->wave + 1 > 30) ? 1 : cheat_menu->wave + 1; break;
                                case 1: cheat_menu->lives = (cheat_menu->lives + 1 > 20) ? 1 : cheat_menu->lives + 1; break;
                                case 2: cheat_menu->missiles = (cheat_menu->missiles + 1 > 99) ? 0 : cheat_menu->missiles + 1; break;
                                case 3: cheat_menu->bombs = (cheat_menu->bombs + 1 > 99) ? 0 : cheat_menu->bombs + 1; break;
                                case 4: cheat_menu->cheat_difficulty = (cheat_menu->cheat_difficulty + 1 > 2) ? 0 : cheat_menu->cheat_difficulty + 1; break;

                            }
                        } else if (event.key.keysym.sym == SDLK_RETURN) {
                            if (cheat_menu->selection == 5) {  // Apply
                                int old_wave = gui->visualizer.comet_buster.current_wave;
                                int new_wave = cheat_menu->wave;
                                bool wave_changed = (old_wave != new_wave);
                                
                                // Apply all cheats
                                gui->visualizer.comet_buster.current_wave = new_wave;
                                gui->visualizer.comet_buster.ship_lives = cheat_menu->lives;
                                gui->visualizer.comet_buster.missile_ammo = cheat_menu->missiles;
                                gui->visualizer.comet_buster.bomb_ammo = cheat_menu->bombs;
                                gui->visualizer.comet_buster.difficulty = cheat_menu->cheat_difficulty;

                                // If wave changed, reset and spawn new wave
                                if (wave_changed) {
                                    SDL_Log("[Comet Busters] [CHEAT] Wave changed from %d to %d - spawning new wave\n", old_wave, new_wave);
                                    
                                    // Clear all entities
                                    gui->visualizer.comet_buster.comet_count = 0;
                                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                                    gui->visualizer.comet_buster.bullet_count = 0;
                                    gui->visualizer.comet_buster.particle_count = 0;
                                    gui->visualizer.comet_buster.missile_count = 0;
                                    
                                    // Spawn the new wave
#ifndef ANDROID
                                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif
                                    SDL_Log("[Comet Busters] [CHEAT] Spawned Wave %d\n", new_wave);
                                } else {
                                    SDL_Log("[Comet Busters] [CHEAT] Wave unchanged (still Wave %d) - just updated Lives/Missiles/Bombs\n", new_wave);
                                }
                                
                                SDL_Log("[Comet Busters] [CHEAT] Applied: Wave=%d, Lives=%d, Missiles=%d, Bombs=%d%s\n",
                                       new_wave, cheat_menu->lives, cheat_menu->missiles, cheat_menu->bombs,
                                       wave_changed ? " (NEW WAVE SPAWNED)" : " (SAME WAVE)");
                                
                                cheat_menu->state = CHEAT_MENU_CLOSED;
                            } else if (cheat_menu->selection == 6) {  // Cancel
                                cheat_menu->state = CHEAT_MENU_CLOSED;
                            }
                        } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                            cheat_menu->state = CHEAT_MENU_CLOSED;
                        }
                        break;  // Don't process other menu input while cheat menu is open
                    }
                    
                    // Regular menu input (only if cheat menu is not open)
                    if (event.key.keysym.sym == SDLK_c && !cheat_menu) {
                        // Can't open cheat without cheat_menu struct
                        SDL_Log("[Comet Busters] [CHEAT] Error: cheat_menu is NULL\n");
                    } else if (event.key.keysym.sym == SDLK_c && cheat_menu) {
                        // Open cheat menu from main menu
                        if (gui->menu_state == 0) {
                            // Initialize cheat menu with current game values
                            cheat_menu->state = CHEAT_MENU_OPEN;
                            cheat_menu->selection = 0;
                            cheat_menu->wave = gui->visualizer.comet_buster.current_wave;
                            cheat_menu->lives = gui->visualizer.comet_buster.ship_lives;
                            cheat_menu->missiles = gui->visualizer.comet_buster.missile_ammo;
                            cheat_menu->bombs = gui->visualizer.comet_buster.bomb_count;
                            SDL_Log("[Comet Busters] [CHEAT] Opening cheat menu (Current: Wave=%d, Lives=%d, Missiles=%d, Bombs=%d)\n",
                                   cheat_menu->wave, cheat_menu->lives, cheat_menu->missiles, cheat_menu->bombs);
                        }
                        break;
                    }
                    
                    // Handle regular menu navigation
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (gui->menu_state == 0) {
                            // Main menu wrapping navigation
                            menu_move_up(gui);

                        } else if (gui->menu_state == 1) {
                            menu_update_difficulty(gui, -1);
                        } else if (gui->menu_state == 3) {
                            menu_move_up(gui);
                        } else if (gui->menu_state == 4) {
                            menu_update_language(gui, -1);
                        } else if (gui->menu_state == 5 || gui->menu_state == 6) {
                            menu_move_up(gui);
                        }
                    } else if (event.key.keysym.sym == SDLK_DOWN) {
                        if (gui->menu_state == 0) {
                            // Main menu wrapping navigation
                            int num_menu_items = 10;  // 0=Continue, 1=New, 2=HighScores, 3=Save, 4=Load, 5=Audio, 6=Language, 7=Help, 8=Fullscreen, 9=Quit
                            int items_per_page = 5;
                            
                            gui->menu_selection = (gui->menu_selection + 1) % num_menu_items;
                            
                            // Adjust scroll offset to keep selection visible
                            int max_scroll = (num_menu_items > items_per_page) ? 
                                            (num_menu_items - items_per_page) : 0;
                            
                            if (gui->menu_selection >= gui->main_menu_scroll_offset + items_per_page) {
                                gui->main_menu_scroll_offset = gui->menu_selection - items_per_page + 1;
                            }
                            
                            if (gui->main_menu_scroll_offset > max_scroll) {
                                gui->main_menu_scroll_offset = max_scroll;
                            }
                        } else if (gui->menu_state == 1) {
                            menu_move_down(gui);
                        } else if (gui->menu_state == 3) {
                            menu_move_down(gui);
                        } else if (gui->menu_state == 4) {
                            menu_update_language(gui, 1);
                        } else if (gui->menu_state == 5 || gui->menu_state == 6) {
                            menu_move_down(gui);
                        }
                    } else if (event.key.keysym.sym == SDLK_LEFT) {
                        if (gui->menu_state == 3) {
                            menu_update_volume(gui, gui->menu_selection, -1);
                        }
                    } else if (event.key.keysym.sym == SDLK_RIGHT) {
                        if (gui->menu_state == 3) {
                            menu_update_volume(gui, gui->menu_selection, 1);
                        }
                    } else if (event.key.keysym.sym == SDLK_c) {
                        // Open cheat menu from main menu
                        if (gui->show_menu && gui->menu_state == 0) {
                            cheat_menu->state = CHEAT_MENU_OPEN;
                            cheat_menu->selection = 0;
                            SDL_Log("[Comet Busters] [CHEAT] Opening cheat menu\n");
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_RETURN) {
                        if (gui->show_help_overlay) {
                            // Close help overlay and return to main menu
                            gui->show_help_overlay = false;
                            gui->help_scroll_offset = 0;
                            gui->menu_state = 0;
                            gui->menu_selection = 5;  // Keep Help selected
                        } else if (gui->menu_state == 0) {
                            switch (gui->menu_selection) {
                                case 0:  // Continue
                                    if(gui->visualizer.comet_buster.ship_lives >0) {
                                         gui->show_menu = false;
                                    }                               
                                    break;
                                case 1:  // New Game - go to difficulty selection
                                    gui->menu_state = 1;
                                    gui->gui_difficulty_level = 1;
                                    break;
                                case 2:  // High Scores
                                    gui->menu_state = 2;
                                    break;
                                case 3:  // Save Game
                                    gui->menu_state = 5;
                                    gui->menu_selection = 0;
                                    break;
                                case 4:  // Load Game
                                    gui->menu_state = 6;
                                    gui->menu_selection = 0;
                                    break;
                                case 5:  // Audio
                                    menu_open_submenu(gui, 3);
                                    break;
                                case 6:  // Language
                                    menu_open_submenu(gui, 4);
                                    break;
                                case 7:  // Help - Show help overlay
                                    gui->show_help_overlay = true;
                                    gui->help_scroll_offset = 0;
                                    break;
                                case 8:  // Fullscreen toggle
                                    gui->fullscreen = !gui->fullscreen;
                                    if (gui->fullscreen) {
                                        SDL_SetWindowFullscreen(gui->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                                    } else {
                                        SDL_SetWindowFullscreen(gui->window, 0);
                                    }
                                    SDL_Log("[Comet Busters] [INPUT] Fullscreen toggled from menu: %s\n", gui->fullscreen ? "ON" : "OFF");
                                    break;
                                case 9:  // Quit
                                    gui->running = false;
                                    break;
                            }
                        } else if (gui->menu_state == 1) {
                            // Start game with selected difficulty
                          
                            comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, gui->gui_difficulty_level-1);
                            
                            if (hs_entry) {
                                hs_entry->state = HIGH_SCORE_ENTRY_NONE;
                                hs_entry->cursor_pos = 0;
                                memset(hs_entry->name_input, 0, sizeof(hs_entry->name_input));
                            }
#ifdef ExternalSound
                           play_intro(gui, gui->visualizer.comet_buster.current_language);
#endif
                            gui->show_menu = false;
                            gui->game_paused = false;
                            gui->finale_music_started = false;
                        } else if (gui->menu_state == 2) {
                            gui->menu_state = 0;
                            gui->menu_selection = 2;
                        } else if (gui->menu_state == 3) {
                            gui->menu_state = 0;
                            gui->menu_selection = 3;
                        } else if (gui->menu_state == 4) {
                            gui->menu_state = 0;
                            gui->menu_selection = 4;
                        } else if (gui->menu_state == 5) {
                            // Save game state
                            if (menu_save_state(gui, gui->menu_selection)) {
                                SDL_Log("[Comet Busters] [MENU] Game saved to slot %d\n", gui->menu_selection);
                            }
                            gui->menu_state = 0;
                            gui->menu_selection = 3;
                        } else if (gui->menu_state == 6) {
                            // Load game state
                            if (menu_load_state(gui, gui->menu_selection)) {
                                SDL_Log("[Comet Busters] [MENU] Game loaded from slot %d\n", gui->menu_selection);
                                gui->show_menu = false;
                            } else {
                                SDL_Log("[Comet Busters] [MENU] Failed to load from slot %d\n", gui->menu_selection);
                            }
                        }
                    }
                } else {
                    // Game is running - handle game input
                    handle_keyboard_input(&event, gui, hs_entry);
                    handle_keyboard_input_special(&event, gui);
                }
                break;
            }
            
            case SDL_KEYUP:
                if (!gui->show_menu && (!hs_entry || hs_entry->state != HIGH_SCORE_ENTRY_ACTIVE)) {
                    handle_keyboard_input(&event, gui, hs_entry);
                }
                break;
            
            case SDL_MOUSEBUTTONDOWN: {
                // Check for splash screen exit on mouse click
                gui->visualizer.mouse_just_moved = true;
                
                // Handle high score entry virtual keyboard clicks
                if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE) {
                    // Convert from window coordinates to game space (1920x1080)
#ifndef ANDROID
                    int mouse_x = (int)((event.button.x / (float)gui->window_width) * 1920.0f);
                    int mouse_y = (int)((event.button.y / (float)gui->window_height) * 1080.0f);
#else
                    int mouse_x = (int)((event.button.x / (float)gui->window_width) * 720.0f);
                    int mouse_y = (int)((event.button.y / (float)gui->window_height) * 480.0f);
#endif                    
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int button_idx = get_keyboard_button_at_pos(mouse_x, mouse_y);
                        if (button_idx >= 0) {
                            int count = 0;
                            KeyboardButton *buttons = get_keyboard_buttons(&count);
                            KeyboardButton *btn = &buttons[button_idx];
                            
                            if (btn->character == '\b') {
                                // Backspace
                                if (hs_entry->cursor_pos > 0) {
                                    hs_entry->cursor_pos--;
                                    hs_entry->name_input[hs_entry->cursor_pos] = '\0';
                                }
                            } else if (btn->character == '\n') {
                                // Submit (Done button)
                                if (hs_entry->cursor_pos > 0) {
                                    high_scores_add(&gui->visualizer.comet_buster, 
                                                   gui->visualizer.comet_buster.score,
                                                   gui->visualizer.comet_buster.current_wave,
                                                   hs_entry->name_input);
                                    high_scores_save(&gui->visualizer.comet_buster);
                                    
                                    hs_entry->state = HIGH_SCORE_ENTRY_SAVED;
                                    gui->visualizer.comet_buster.game_over = false;
                                    gui->show_menu = true;
                                    gui->menu_state = 2;  // Show high scores
                                    gui->menu_selection = 2;
                                }
                            } else if (btn->character == 'C' && btn->is_special) {
                                // Clear button
                                hs_entry->cursor_pos = 0;
                                memset(hs_entry->name_input, 0, sizeof(hs_entry->name_input));
                            } else {
                                // Regular character (letter or space)
                                add_character_to_input(hs_entry, btn->character);
                            }
                            break;  // Don't process other events while in high score entry
                        }
                    }
                    break;  // Don't process other input while in high score entry
                }
                
                if (gui->visualizer.comet_buster.splash_screen_active) {
                    SDL_Log("[Comet Busters] [SPLASH] User clicked - exiting splash screen\n");
                    
                    // Stop the intro music
                    audio_stop_music(&gui->audio);
                    
                    // Exit the splash screen
                    gui->visualizer.comet_buster.splash_screen_active = false;
                    
                    // Clear the board completely
                    gui->visualizer.comet_buster.comet_count = 0;
                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                    gui->visualizer.comet_buster.bullet_count = 0;
                    gui->visualizer.comet_buster.particle_count = 0;
                    gui->visualizer.comet_buster.floating_text_count = 0;
                    gui->visualizer.comet_buster.canister_count = 0;
                    gui->visualizer.comet_buster.missile_count = 0;
                    gui->visualizer.comet_buster.missile_pickup_count = 0;
                    gui->visualizer.comet_buster.bomb_count = 0;
                    gui->visualizer.comet_buster.bomb_pickup_count = 0;
                    gui->visualizer.comet_buster.score = 0;
                    gui->visualizer.comet_buster.score_multiplier = 1.0;
                    gui->visualizer.comet_buster.game_over = false;
                    gui->visualizer.comet_buster.game_won = false;
                    
                    // Spawn wave 1
#ifndef ANDROID
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    audio_play_random_music(&gui->audio);
                    SDL_Log("[Comet Busters] [SPLASH] Started gameplay music\n");
#endif
                    break;
                }
                
                if (gui->show_menu) {
                    // Convert from window coordinates to game space (1920x1080)
                    // This matches how render_frame uses glOrtho for scaling
#ifndef ANDROID
                    int mouse_x = (int)((event.button.x / (float)gui->window_width) * 1920.0f);
                    int mouse_y = (int)((event.button.y / (float)gui->window_height) * 1080.0f);
#else
                    int mouse_x = (int)((event.button.x / (float)gui->window_width) * 720.0f);
                    int mouse_y = (int)((event.button.y / (float)gui->window_height) * 480.0f);
#endif                    
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // Main menu buttons
                        if (gui->menu_state == 0) {
                            const int menu_y_start = 300;  // MUST MATCH render code
                            const int menu_spacing = 110;  // MUST MATCH render code
                            const int menu_width = 400;
                            const int menu_height = 60;
                            const int items_per_page = 5;
#ifndef ANDROID
                            const int menu_x = (1920 - menu_width) / 2;
#else
                            const int menu_x = (720 - menu_width) / 2;
#endif                            
                            for (int i = 0; i < 10; i++) {  // 0=Continue, 1=New, 2=High Scores, 3=Save, 4=Load, 5=Audio, 6=Language, 7=Help, 8=Fullscreen, 9=Quit
                                // Only check items that are visible on screen
                                if (i < gui->main_menu_scroll_offset || i >= gui->main_menu_scroll_offset + items_per_page) {
                                    continue;  // Skip items outside visible area
                                }
                                
                                // Calculate Y position accounting for scroll offset (MUST MATCH render code)
                                int display_index = i - gui->main_menu_scroll_offset;
                                int menu_y = menu_y_start + (display_index * menu_spacing);
                                
                                if (mouse_x >= menu_x && mouse_x <= menu_x + menu_width &&
                                    mouse_y >= menu_y && mouse_y <= menu_y + menu_height) {
                                    gui->menu_selection = i;
                                    switch (gui->menu_selection) {
                                        case 0:  // Continue
                                            if(gui->visualizer.comet_buster.ship_lives > 0) {
                                                gui->show_menu = false;
                                            }
                                            break;
                                        case 1:  // New Game
                                            gui->menu_state = 1;
                                            gui->gui_difficulty_level = 1;
                                            break;
                                        case 2:  // High Scores
                                            gui->menu_state = 2;
                                            break;
                                        case 3:  // Save
                                            gui->menu_state = 5;  // Save state menu
                                            gui->menu_selection = 0;
                                            break;
                                        case 4:  // Load
                                            gui->menu_state = 6;  // Load state menu
                                            gui->menu_selection = 0;
                                            break;
                                        case 5:  // Audio
                                            menu_open_submenu(gui, 3);
                                            break;
                                        case 6:  // Language
                                            menu_open_submenu(gui, 4);
                                            break;
                                        case 7:  // Help
                                            gui->show_help_overlay = true;
                                            gui->help_scroll_offset = 0;
                                            break;
                                        case 8:  // Fullscreen toggle
                                            gui->fullscreen = !gui->fullscreen;
                                            if (gui->fullscreen) {
                                                SDL_SetWindowFullscreen(gui->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                                            } else {
                                                SDL_SetWindowFullscreen(gui->window, 0);
                                            }
                                            SDL_Log("[Comet Busters] [INPUT] Fullscreen toggled from menu: %s\n", gui->fullscreen ? "ON" : "OFF");
                                            break;
                                        case 9:  // Quit
                                            gui->running = false;
                                            break;
                                    }
                                }
                            }
                        }
                        // Difficulty selection buttons
                        else if (gui->menu_state == 1) {
                            const int diff_y_start = 350;
                            const int diff_spacing = 120;
                            const int diff_width = 400;
                            const int diff_height = 60;
#ifndef ANDROID
                            const int diff_x = (1920 - diff_width) / 2;
#else
                            const int diff_x = (720 - diff_width) / 2;
#endif                            
                            for (int i = 0; i < 3; i++) {
                                int diff_y = diff_y_start + (i * diff_spacing);
                                if (mouse_x >= diff_x && mouse_x <= diff_x + diff_width &&
                                    mouse_y >= diff_y && mouse_y <= diff_y + diff_height) {
                                    gui->gui_difficulty_level = i + 1;
                                    // Start game with selected difficulty
                                    comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, gui->gui_difficulty_level-1);
#ifdef ExternalSound
                                    play_intro(gui, gui->visualizer.comet_buster.current_language);
                                    //audio_play_intro_music(&gui->audio, "music/intro.mp3");
#endif
                                    gui->show_menu = false;
                                    gui->game_paused = false;
                                    gui->finale_music_started = false;
                                }
                            }
                        }
                        // High Scores - click to go back
                        else if (gui->menu_state == 2) {
                            gui->menu_state = 0;
                            gui->menu_selection = 2;
                        }
                        // Audio menu buttons
                        else if (gui->menu_state == 3) {
                            const int audio_y_start = 350;
                            const int audio_spacing = 150;
                            
                            for (int i = 0; i < 2; i++) {
                                int option_y = audio_y_start + (i * audio_spacing);
                                if (mouse_y >= option_y && mouse_y <= option_y + 50) {
                                    gui->menu_selection = i;
                                }
                            }
                        }
                        // Language menu buttons
                        else if (gui->menu_state == 4) {
                            const int lang_y_start = 300;  // MUST MATCH render code
                            const int lang_spacing = 110;  // MUST MATCH render code
                            const int lang_width = 400;
                            const int lang_height = 60;
#ifndef ANDROID
                            const int lang_x = (1920 - lang_width) / 2;
#else
                            const int lang_x = (720 - lang_width) / 2;
#endif
                            const int items_per_page = 4;
                            
                            int num_languages = sizeof(wlanguagename) / sizeof(wlanguagename[0]);
                            
                            for (int i = 0; i < num_languages; i++) {
                                // Only check items that are visible on screen
                                if (i < gui->lang_menu_scroll_offset || i >= gui->lang_menu_scroll_offset + items_per_page) {
                                    continue;  // Skip items outside visible area
                                }
                                
                                // Calculate Y position accounting for scroll offset
                                int display_index = i - gui->lang_menu_scroll_offset;
                                int lang_y = lang_y_start + (display_index * lang_spacing);
                                
                                if (mouse_x >= lang_x && mouse_x <= lang_x + lang_width &&
                                    mouse_y >= lang_y && mouse_y <= lang_y + lang_height) {
                                    gui->visualizer.comet_buster.current_language = i;
                                    // SAVE PREFERENCES
                                    gui->preferences.language = i;
                                    preferences_save(&gui->preferences);
                                    // Return to main menu
                                    gui->menu_state = 0;
                                    gui->menu_selection = 4;
                                }
                            }
                        }
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        gui->visualizer.mouse_right_pressed = true;
                        
                        // Right-click to restart if game over
                        if (gui->visualizer.comet_buster.game_over) {
                            comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, gui->gui_difficulty_level-1);
                            gui->game_paused = false;
                            gui->finale_music_started = false;
#ifdef ExternalSound
                            play_intro(gui, gui->visualizer.comet_buster.current_language);
                            //audio_play_intro_music(&gui->audio, "music/intro.mp3");
#endif
                        }
                    }
                } else {
                    // Game is running - handle mouse input for game
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        gui->visualizer.mouse_left_pressed = true;
                    } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                        gui->visualizer.mouse_middle_pressed = true;
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        gui->visualizer.mouse_right_pressed = true;
                    }
                }
                break;
            }
            
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    gui->visualizer.mouse_left_pressed = false;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    gui->visualizer.mouse_middle_pressed = false;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    gui->visualizer.mouse_right_pressed = false;
                }
                break;
            
            case SDL_MOUSEMOTION: {
                gui->visualizer.mouse_just_moved = true;
                // Convert window pixel coordinates to game logical coordinates
                // Window fills entire screen, so scaling is direct
#ifndef ANDROID
                gui->visualizer.mouse_x = (int)(event.motion.x * 1920.0f / gui->window_width);
                gui->visualizer.mouse_y = (int)(event.motion.y * 1080.0f / gui->window_height);
#else
                gui->visualizer.mouse_x = (int)(event.motion.x * 720.0f / gui->window_width);
                gui->visualizer.mouse_y = (int)(event.motion.y * 480.0f / gui->window_height);
#endif
                break;
            }
            
            case SDL_MOUSEWHEEL:
                gui->visualizer.mouse_just_moved = true;
                gui->visualizer.scroll_direction = event.wheel.y;
                
                // Handle menu scrolling with mouse wheel
                if (gui->show_menu) {
                    if (gui->menu_state == 0) {
                        // Main menu scrolling
                        int num_menu_items = 10;
                        int items_per_page = 5;
                        
                        if (event.wheel.y > 0) {
                            // Scroll up - previous item
                            gui->menu_selection = (gui->menu_selection - 1 + num_menu_items) % num_menu_items;
                        } else if (event.wheel.y < 0) {
                            // Scroll down - next item
                            gui->menu_selection = (gui->menu_selection + 1) % num_menu_items;
                        }
                        
                        // Auto-adjust scroll offset to keep selection visible
                        if (gui->menu_selection < gui->main_menu_scroll_offset) {
                            gui->main_menu_scroll_offset = gui->menu_selection;
                        }
                        int max_scroll = (num_menu_items > items_per_page) ? (num_menu_items - items_per_page) : 0;
                        if (gui->menu_selection >= gui->main_menu_scroll_offset + items_per_page) {
                            gui->main_menu_scroll_offset = gui->menu_selection - items_per_page + 1;
                        }
                        if (gui->main_menu_scroll_offset > max_scroll) {
                            gui->main_menu_scroll_offset = max_scroll;
                        }
                    } else if (gui->menu_state == 4) {
                        // Language menu scrolling
                        int num_languages = sizeof(wlanguagename) / sizeof(wlanguagename[0]);
                        int items_per_page = 4;
                        
                        if (event.wheel.y > 0) {
                            // Scroll up - previous language
                            gui->visualizer.comet_buster.current_language = 
                                (gui->visualizer.comet_buster.current_language - 1 + num_languages) % num_languages;
                        } else if (event.wheel.y < 0) {
                            // Scroll down - next language
                            gui->visualizer.comet_buster.current_language = 
                                (gui->visualizer.comet_buster.current_language + 1) % num_languages;
                        }
                        
                        // Auto-adjust scroll offset to keep selection visible
                        int current_lang = gui->visualizer.comet_buster.current_language;
                        if (current_lang < gui->lang_menu_scroll_offset) {
                            gui->lang_menu_scroll_offset = current_lang;
                        }
                        int max_scroll = (num_languages > items_per_page) ? (num_languages - items_per_page) : 0;
                        if (current_lang >= gui->lang_menu_scroll_offset + items_per_page) {
                            gui->lang_menu_scroll_offset = current_lang - items_per_page + 1;
                        }
                        if (gui->lang_menu_scroll_offset > max_scroll) {
                            gui->lang_menu_scroll_offset = max_scroll;
                        }
                        
                        // SAVE PREFERENCES
                        gui->preferences.language = gui->visualizer.comet_buster.current_language;
                        preferences_save(&gui->preferences);
                    }
                }
                break;
            
            // JOYSTICK BUTTON EVENTS
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP: {
                bool pressed = (event.type == SDL_JOYBUTTONDOWN);
                int button = event.jbutton.button;
                
                if (pressed) {
                    SDL_Log("[Comet Busters] [JOYSTICK] *** Button %d PRESSED (show_menu=%d, splash_active=%d, menu_state=%d) ***\n", 
                            button, gui->show_menu, gui->visualizer.comet_buster.splash_screen_active, 
                            gui->show_menu ? gui->menu_state : -1);
                }
                
                // Handle cheat menu with joystick (takes priority over game input)
                if (cheat_menu && cheat_menu->state == CHEAT_MENU_OPEN && pressed) {
                    switch (button) {
                        case 0:  // A button - Apply/Select
                            if (cheat_menu->selection == 5) {  // Apply
                                int old_wave = gui->visualizer.comet_buster.current_wave;
                                int new_wave = cheat_menu->wave;
                                bool wave_changed = (old_wave != new_wave);
                                
                                // Apply all cheats
                                gui->visualizer.comet_buster.current_wave = new_wave;
                                gui->visualizer.comet_buster.ship_lives = cheat_menu->lives;
                                gui->visualizer.comet_buster.missile_ammo = cheat_menu->missiles;
                                gui->visualizer.comet_buster.bomb_ammo = cheat_menu->bombs;
                                gui->visualizer.comet_buster.difficulty = cheat_menu->cheat_difficulty;

                                // If wave changed, reset and spawn new wave
                                if (wave_changed) {
                                    SDL_Log("[Comet Busters] [CHEAT] Wave changed from %d to %d - spawning new wave\n", old_wave, new_wave);
                                    
                                    // Clear all entities
                                    gui->visualizer.comet_buster.comet_count = 0;
                                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                                    gui->visualizer.comet_buster.bullet_count = 0;
                                    gui->visualizer.comet_buster.particle_count = 0;
                                    gui->visualizer.comet_buster.missile_count = 0;
                                    
                                    // Spawn the new wave
#ifndef ANDROID
                                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif
                                    SDL_Log("[Comet Busters] [CHEAT] Spawned Wave %d\n", new_wave);
                                } else {
                                    SDL_Log("[Comet Busters] [CHEAT] Wave unchanged - just updated Lives/Missiles/Bombs\n");
                                }
                                
                                SDL_Log("[Comet Busters] [CHEAT] Applied: Wave=%d, Lives=%d, Missiles=%d, Bombs=%d\n",
                                       new_wave, cheat_menu->lives, cheat_menu->missiles, cheat_menu->bombs);
                                
                                cheat_menu->state = CHEAT_MENU_CLOSED;
                            } else if (cheat_menu->selection == 6) {  // Cancel
                                cheat_menu->state = CHEAT_MENU_CLOSED;
                            }
                            break;
                        
                        case 1:  // B button - Cancel
                            cheat_menu->state = CHEAT_MENU_CLOSED;
                            break;
                        
                        case 5:  // RB / R1 - Close cheat menu (toggle)
                            cheat_menu->state = CHEAT_MENU_CLOSED;
                            SDL_Log("[Comet Busters] [CHEAT] Cheat menu closed via joystick button\n");
                            break;
                    }
                    // Don't process other buttons while cheat menu is open
                    break;
                }
                
                // Handle splash screen exit with joystick button
                if (gui->visualizer.comet_buster.splash_screen_active && pressed) {
                    SDL_Log("[Comet Busters] [SPLASH] Joystick button %d pressed while splash active - exiting splash screen\n", button);
                    
                    // Stop the intro music
                    SDL_Log("[Comet Busters] [SPLASH] Calling audio_stop_music()...\n");
                    audio_stop_music(&gui->audio);
                    
                    // Exit the splash screen
                    gui->visualizer.comet_buster.splash_screen_active = false;
                    
                    // Clear the board completely
                    gui->visualizer.comet_buster.comet_count = 0;
                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                    gui->visualizer.comet_buster.bullet_count = 0;
                    gui->visualizer.comet_buster.particle_count = 0;
                    gui->visualizer.comet_buster.floating_text_count = 0;
                    gui->visualizer.comet_buster.canister_count = 0;
                    gui->visualizer.comet_buster.missile_count = 0;
                    gui->visualizer.comet_buster.missile_pickup_count = 0;
                    gui->visualizer.comet_buster.bomb_count = 0;
                    gui->visualizer.comet_buster.bomb_pickup_count = 0;
                    gui->visualizer.comet_buster.score = 0;
                    gui->visualizer.comet_buster.score_multiplier = 1.0;
                    gui->visualizer.comet_buster.game_over = false;
                    gui->visualizer.comet_buster.game_won = false;
                    
                    // Spawn wave 1
#ifndef ANDROID
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    SDL_Log("[Comet Busters] [SPLASH] Calling audio_play_random_music()...\n");
                    audio_play_random_music(&gui->audio);
                    SDL_Log("[Comet Busters] [SPLASH] Started gameplay music\n");
#endif
                    SDL_Log("[Comet Busters] [SPLASH] Exiting splash screen handler for button %d\n", button);
                    break;  // Don't process other input while exiting splash
                }
                
                // Handle high score entry with joystick
                if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE && pressed) {
                    int count = 0;
                    KeyboardButton *buttons = get_keyboard_buttons(&count);
                    KeyboardButton *current_btn = &buttons[hs_entry->kb_selected_index];
                    
                    switch (button) {
                        case 0:  // A button - select current key
                            if (current_btn->character == '\b') {
                                // Backspace
                                if (hs_entry->cursor_pos > 0) {
                                    hs_entry->cursor_pos--;
                                    hs_entry->name_input[hs_entry->cursor_pos] = '\0';
                                }
                            } else if (current_btn->character == '\n') {
                                // Submit (Done button)
                                if (hs_entry->cursor_pos > 0) {
                                    high_scores_add(&gui->visualizer.comet_buster, 
                                                   gui->visualizer.comet_buster.score,
                                                   gui->visualizer.comet_buster.current_wave,
                                                   hs_entry->name_input);
                                    high_scores_save(&gui->visualizer.comet_buster);
                                    
                                    hs_entry->state = HIGH_SCORE_ENTRY_SAVED;
                                    gui->visualizer.comet_buster.game_over = false;
                                    gui->show_menu = true;
                                    gui->menu_state = 2;  // Show high scores
                                    gui->menu_selection = 2;
                                }
                            } else if (current_btn->character == 'C' && current_btn->is_special) {
                                // Clear button
                                hs_entry->cursor_pos = 0;
                                memset(hs_entry->name_input, 0, sizeof(hs_entry->name_input));
                            } else {
                                // Regular character
                                add_character_to_input(hs_entry, current_btn->character);
                            }
                            break;
                        case 1:  // B button - cancel/back
                            // Could implement back to menu option here
                            break;
                    }
                    break;  // Don't process other input while in high score entry
                }
                
                // Handle Start button BEFORE checking show_menu - works in all cases
                if (button >= 6 && button <= 9) {  // START - buttons 6, 7, 8, 9
                    SDL_Log("[Comet Busters] [JOYSTICK] Start button (button %d) pressed=%d\n", button, pressed);
                    if (pressed) {
                        SDL_Log("[Comet Busters] [MENU] show_menu=%d, menu_state=%d, ship_lives=%d\n", 
                                gui->show_menu, gui->menu_state, gui->visualizer.comet_buster.ship_lives);
                        
                        if (!gui->show_menu) {
                            // Menu not shown - open it
                            SDL_Log("[Comet Busters] [MENU] Start: Menu closed, opening menu\n");
                            gui->show_menu = true;
                            menu_open_submenu(gui, 0);  // Select Continue by default
                            SDL_Log("[Comet Busters] [MENU] Start button opened menu\n");
                        } else if (gui->show_menu && gui->menu_state == 0) {
                            // Menu is shown on main menu - treat Start like pressing Continue
                            SDL_Log("[Comet Busters] [MENU] Start: Menu open on main menu, pressing Continue\n");
                            if (gui->visualizer.comet_buster.ship_lives > 0) {
                                gui->show_menu = false;
                                SDL_Log("[Comet Busters] [MENU] Start button closed menu (Continue)\n");
                            } else {
                                SDL_Log("[Comet Busters] [MENU] Start: No lives, can't continue\n");
                            }
                        } else {
                            // Menu is shown but in a submenu - go back to main menu
                            SDL_Log("[Comet Busters] [MENU] Start: Menu open in submenu, returning to main\n");
                            menu_open_submenu(gui, 0);
                            SDL_Log("[Comet Busters] [MENU] Start button returned to main menu\n");
                        }
                    }
                    break;  // Handle Start button and skip the show_menu block
                }
                
                // Handle Button 5 (RB/R1) - Cheat Menu
                if (button == 5 && pressed) {
                    SDL_Log("[Comet Busters] [JOYSTICK] Button 5 pressed - cheat_menu=%p\n", cheat_menu);
                    
                    if (!cheat_menu) {
                        SDL_Log("[Comet Busters] [CHEAT] Error: cheat_menu is NULL\n");
                    } else if (gui->show_menu && gui->menu_state == 0) {
                        // Open cheat menu from main menu
                        SDL_Log("[Comet Busters] [CHEAT] Opening cheat menu\n");
                        cheat_menu->state = CHEAT_MENU_OPEN;
                        cheat_menu->selection = 0;
                        cheat_menu->wave = gui->visualizer.comet_buster.current_wave;
                        cheat_menu->lives = gui->visualizer.comet_buster.ship_lives;
                        cheat_menu->missiles = gui->visualizer.comet_buster.missile_ammo;
                        cheat_menu->bombs = gui->visualizer.comet_buster.bomb_count;
                        cheat_menu->cheat_difficulty = gui->visualizer.comet_buster.difficulty;
                        SDL_Log("[Comet Busters] [CHEAT] Cheat menu opened: Wave=%d, Lives=%d, Missiles=%d, Bombs=%d\n",
                                cheat_menu->wave, cheat_menu->lives, cheat_menu->missiles, cheat_menu->bombs);
                    }
                    break;  // Don't process other input after handling button 5
                }
                
                if (gui->show_menu) {
                    // Menu navigation with joystick
                    switch (button) {
                        case 0:  // A button - select/confirm
                            if (pressed) {
                                if (gui->show_help_overlay) {
                                    // Close help overlay and return to main menu
                                    gui->show_help_overlay = false;
                                    gui->help_scroll_offset = 0;
                                    gui->menu_state = 0;
                                    gui->menu_selection = 5;  // Keep Help selected
                                } else if (gui->menu_state == 0) {
                                    switch (gui->menu_selection) {
                                        case 0:  // Continue
                                            if(gui->visualizer.comet_buster.ship_lives > 0) {
                                                gui->show_menu = false;
                                            }
                                            break;
                                        case 1:  // New Game
                                            gui->menu_state = 1;
                                            gui->gui_difficulty_level = 1;
                                            break;
                                        case 2:  // High Scores
                                            gui->menu_state = 2;
                                            break;
                                        case 3:  // Save Game
                                            gui->menu_state = 5;
                                            gui->menu_selection = 0;
                                            break;
                                        case 4:  // Load Game
                                            gui->menu_state = 6;
                                            gui->menu_selection = 0;
                                            break;
                                        case 5:  // Audio
                                            menu_open_submenu(gui, 3);
                                            break;
                                        case 6:  // Language
                                            menu_open_submenu(gui, 4);
                                            break;
                                        case 7:  // Help
                                            gui->show_help_overlay = true;
                                            gui->help_scroll_offset = 0;
                                            break;
                                        case 8:  // Fullscreen toggle
                                            gui->fullscreen = !gui->fullscreen;
                                            if (gui->fullscreen) {
                                                SDL_SetWindowFullscreen(gui->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                                            } else {
                                                SDL_SetWindowFullscreen(gui->window, 0);
                                            }
                                            SDL_Log("[Comet Busters] [INPUT] Fullscreen toggled from menu: %s\n", gui->fullscreen ? "ON" : "OFF");
                                            break;
                                        case 9:  // Quit
                                            gui->running = false;
                                            break;
                                    }
                                } else if (gui->menu_state == 1) {
                                    // Start game with selected difficulty
                                    comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, gui->gui_difficulty_level-1);
                                    
                                    if (hs_entry) {
                                        hs_entry->state = HIGH_SCORE_ENTRY_NONE;
                                        hs_entry->cursor_pos = 0;
                                        memset(hs_entry->name_input, 0, sizeof(hs_entry->name_input));
                                    }
#ifdef ExternalSound
                                    play_intro(gui, gui->visualizer.comet_buster.current_language);
#endif
                                    gui->show_menu = false;
                                    gui->game_paused = false;
                                    gui->finale_music_started = false;
                                } else if (gui->menu_state == 2) {
                                    // High Scores - return to main menu
                                    gui->menu_state = 0;
                                    gui->menu_selection = 2;
                                } else if (gui->menu_state == 3) {
                                    // Audio - return to main menu
                                    gui->menu_state = 0;
                                    gui->menu_selection = 5;
                                } else if (gui->menu_state == 4) {
                                    // Language - return to main menu
                                    gui->menu_state = 0;
                                    gui->menu_selection = 6;
                                } else if (gui->menu_state == 5) {
                                    // Save game state
                                    if (menu_save_state(gui, gui->menu_selection)) {
                                        SDL_Log("[Comet Busters] [MENU] Game saved to slot %d\n", gui->menu_selection);
                                    }
                                    gui->menu_state = 0;
                                    gui->menu_selection = 3;
                                } else if (gui->menu_state == 6) {
                                    // Load game state
                                    if (menu_load_state(gui, gui->menu_selection)) {
                                        SDL_Log("[Comet Busters] [MENU] Game loaded from slot %d\n", gui->menu_selection);
                                        gui->show_menu = false;
                                    } else {
                                        SDL_Log("[Comet Busters] [MENU] Failed to load from slot %d\n", gui->menu_selection);
                                    }
                                }
                            }
                            break;
                        case 1:  // B button - back/cancel
                            if (pressed) {
                                if (gui->show_help_overlay) {
                                    gui->show_help_overlay = false;
                                    gui->help_scroll_offset = 0;
                                    gui->menu_state = 0;
                                    gui->menu_selection = 5;
                                } else if (gui->menu_state != 0) {
                                    int prev_state = gui->menu_state;
                                    gui->menu_state = 0;
                                    gui->menu_selection = (prev_state == 4) ? 4 : 1;
                                }
                            }
                            break;
                    }
                } else {
                    // In-game joystick controls
                    gui->visualizer.mouse_just_moved = false;  // Disable mouse following
                    
                    switch (button) {
                        case 0:  // A - Fire (like Z key)
                            gui->visualizer.key_z_pressed = pressed;
                            break;
                        case 1:  // B - Boost (like SPACE/X)
                            gui->visualizer.key_x_pressed = pressed;
                            break;
                        case 2:  // X - Missiles/Bullets toggle (like Q)
                            gui->visualizer.key_q_pressed = pressed;
                            break;
                        case 3:  // Y - Alternative fire (like CTRL)
                            gui->visualizer.key_ctrl_pressed = pressed;
                            break;
                        case 4:  // LB / L1 - Pause (like P)
                            if (pressed) {
                                if (!gui->visualizer.comet_buster.game_over && 
                                    !gui->visualizer.comet_buster.splash_screen_active &&
                                    !gui->visualizer.comet_buster.finale_splash_active) {
                                    gui->game_paused = !gui->game_paused;
                                    if (gui->game_paused) {
                                        audio_stop_music(&gui->audio);
                                        SDL_Log("[Comet Busters] [PAUSE] Game paused via joystick\n");
                                    } else {
                                        SDL_Log("[Comet Busters] [RESUME] Game resumed via joystick\n");
#ifdef ExternalSound
                                        if (!gui->show_menu) {
                                            audio_play_random_music(&gui->audio);
                                        }
#endif
                                    }
                                }
                            }
                            break;
                        case 5:  // RB / R1 - Cheat Menu (like C on keyboard)
                            if (pressed) {
                                if (cheat_menu) {
                                    if (cheat_menu->state == CHEAT_MENU_CLOSED) {
                                        cheat_menu->state = CHEAT_MENU_OPEN;
                                        SDL_Log("[Comet Busters] [CHEAT] Cheat menu opened via joystick button\n");
                                    } else {
                                        cheat_menu->state = CHEAT_MENU_CLOSED;
                                        SDL_Log("[Comet Busters] [CHEAT] Cheat menu closed via joystick button\n");
                                    }
                                } else {
                                    SDL_Log("[Comet Busters] [CHEAT] Error: cheat_menu is NULL\n");
                                }
                            }
                            break;
                    }
                }
                break;
            }
            
            // JOYSTICK AXIS EVENTS (analog sticks and triggers)
            case SDL_JOYAXISMOTION: {
                int axis = event.jaxis.axis;
                int value = event.jaxis.value;
                const int AXIS_THRESHOLD = 16384;  // ~25% of max range
                const uint32_t AXIS_THROTTLE_MS = 50;  // Fast response (50ms = ~20 inputs/second)
                
                // Handle splash screen exit with left stick movement
                if (gui->visualizer.comet_buster.splash_screen_active && (abs(value) > AXIS_THRESHOLD)) {
                    SDL_Log("[Comet Busters] [SPLASH] Joystick axis %d moved (value=%d) while splash active - exiting splash screen\n", axis, value);
                    
                    // Stop the intro music
                    SDL_Log("[Comet Busters] [SPLASH] Calling audio_stop_music()...\n");
                    audio_stop_music(&gui->audio);
                    
                    // Exit the splash screen
                    gui->visualizer.comet_buster.splash_screen_active = false;
                    
                    // Clear the board completely
                    gui->visualizer.comet_buster.comet_count = 0;
                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                    gui->visualizer.comet_buster.bullet_count = 0;
                    gui->visualizer.comet_buster.particle_count = 0;
                    gui->visualizer.comet_buster.floating_text_count = 0;
                    gui->visualizer.comet_buster.canister_count = 0;
                    gui->visualizer.comet_buster.missile_count = 0;
                    gui->visualizer.comet_buster.missile_pickup_count = 0;
                    gui->visualizer.comet_buster.bomb_count = 0;
                    gui->visualizer.comet_buster.bomb_pickup_count = 0;
                    gui->visualizer.comet_buster.score = 0;
                    gui->visualizer.comet_buster.score_multiplier = 1.0;
                    gui->visualizer.comet_buster.game_over = false;
                    gui->visualizer.comet_buster.game_won = false;
                    
                    // Spawn wave 1
#ifndef ANDROID
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    SDL_Log("[Comet Busters] [SPLASH] Calling audio_play_random_music()...\n");
                    audio_play_random_music(&gui->audio);
                    SDL_Log("[Comet Busters] [SPLASH] Started gameplay music\n");
#endif
                    SDL_Log("[Comet Busters] [SPLASH] Axis splash exit complete\n");
                    break;  // Don't process other input while exiting splash
                }
                
                // Handle high score entry navigation with joystick
                if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE) {
                    uint32_t current_time = SDL_GetTicks();
                    bool should_process = (current_time - gui->last_joystick_axis_time) >= AXIS_THROTTLE_MS;
                    
                    if (axis == 0) {
                        // Left stick X-axis (or D-pad) - horizontal keyboard navigation
                        int current_state = (value < -AXIS_THRESHOLD) ? -1 : (value > AXIS_THRESHOLD) ? 1 : 0;
                        
                        if (current_state != gui->last_axis_0_state && should_process) {
                            int count = 0;
                            get_keyboard_buttons(&count);
                            if (current_state < 0) {
                                // Moving left - same as arrow key
                                int row = 0;
                                int row_starts[] = {0, 10, 19, 26, 30};
                                int row_sizes[] = {10, 9, 7, 4};
                                
                                for (int r = 0; r < 4; r++) {
                                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                        row = r;
                                        break;
                                    }
                                }
                                
                                int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                                if (pos_in_row > 0) {
                                    hs_entry->kb_selected_index--;
                                } else {
                                    hs_entry->kb_selected_index = row_starts[row] + row_sizes[row] - 1;
                                }
                            } else if (current_state > 0) {
                                // Moving right - same as arrow key
                                int row = 0;
                                int row_starts[] = {0, 10, 19, 26, 30};
                                int row_sizes[] = {10, 9, 7, 4};
                                
                                for (int r = 0; r < 4; r++) {
                                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                        row = r;
                                        break;
                                    }
                                }
                                
                                int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                                if (pos_in_row < row_sizes[row] - 1) {
                                    hs_entry->kb_selected_index++;
                                } else {
                                    hs_entry->kb_selected_index = row_starts[row];
                                }
                            }
                            gui->last_axis_0_state = current_state;
                            gui->last_joystick_axis_time = current_time;
                        }
                    } else if (axis == 1) {
                        // Left stick Y-axis (or D-pad vertical) - vertical keyboard navigation
                        int current_state = (value < -AXIS_THRESHOLD) ? -1 : (value > AXIS_THRESHOLD) ? 1 : 0;
                        
                        if (current_state != gui->last_axis_1_state && should_process) {
                            int count = 0;
                            get_keyboard_buttons(&count);
                            if (current_state < 0) {
                                // Moving up
                                int row = 0;
                                int row_starts[] = {0, 10, 19, 26, 30};
                                
                                for (int r = 0; r < 4; r++) {
                                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                        row = r;
                                        break;
                                    }
                                }
                                
                                if (row > 0) {
                                    int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                                    int new_row = row - 1;
                                    int row_sizes[] = {10, 9, 7, 4};
                                    if (pos_in_row >= row_sizes[new_row]) {
                                        pos_in_row = row_sizes[new_row] - 1;
                                    }
                                    hs_entry->kb_selected_index = row_starts[new_row] + pos_in_row;
                                }
                            } else if (current_state > 0) {
                                // Moving down
                                int row = 0;
                                int row_starts[] = {0, 10, 19, 26, 30};
                                
                                for (int r = 0; r < 4; r++) {
                                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                        row = r;
                                        break;
                                    }
                                }
                                
                                if (row < 3) {
                                    int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                                    int new_row = row + 1;
                                    int row_sizes[] = {10, 9, 7, 4};
                                    if (pos_in_row >= row_sizes[new_row]) {
                                        pos_in_row = row_sizes[new_row] - 1;
                                    }
                                    hs_entry->kb_selected_index = row_starts[new_row] + pos_in_row;
                                }
                            }
                            gui->last_axis_1_state = current_state;
                            gui->last_joystick_axis_time = current_time;
                        }
                    }
                    break;  // Don't process other input while in high score entry
                }
                
                if (gui->show_menu) {
                    // Menu navigation with D-pad/left stick (with throttling)
                    uint32_t current_time = SDL_GetTicks();
                    bool should_process = (current_time - gui->last_joystick_axis_time) >= AXIS_THROTTLE_MS;
                    
                    if (axis == 0) {
                        // Left stick X-axis (or D-pad)
                        int current_state = (value < -AXIS_THRESHOLD) ? -1 : (value > AXIS_THRESHOLD) ? 1 : 0;
                        
                        if (current_state != gui->last_axis_0_state && should_process) {
                            if (current_state < 0) {
                                // Moving left in menu
                                if (gui->menu_state == 3) {
                                    // Audio menu - decrease volume
                                    menu_update_volume(gui, gui->menu_selection, -1);
                                }
                            } else if (current_state > 0) {
                                // Moving right in menu
                                if (gui->menu_state == 3) {
                                    // Audio menu - increase volume
                                    menu_update_volume(gui, gui->menu_selection, 1);
                                }
                            }
                            gui->last_axis_0_state = current_state;
                            gui->last_joystick_axis_time = current_time;
                        }
                    } else if (axis == 1) {
                        // Left stick Y-axis (or D-pad vertical)
                        int current_state = (value < -AXIS_THRESHOLD) ? -1 : (value > AXIS_THRESHOLD) ? 1 : 0;
                        
                        if (current_state != gui->last_axis_1_state && should_process) {
                            if (current_state < 0) {
                                // Moving up in menu
                                if (gui->menu_state == 0) {
                                    int num_menu_items = 10;
                                    gui->menu_selection = (gui->menu_selection - 1 + num_menu_items) % num_menu_items;
                                    if (gui->menu_selection < gui->main_menu_scroll_offset) {
                                        gui->main_menu_scroll_offset = gui->menu_selection;
                                    }
                                } else if (gui->menu_state == 1) {
                                    menu_update_difficulty(gui, -1);
                                } else if (gui->menu_state == 3) {
                                    menu_move_up(gui);
                                } else if (gui->menu_state == 4) {
                                    menu_update_language(gui, -1);
                                } else if (gui->menu_state == 5 || gui->menu_state == 6) {
                                    menu_move_up(gui);
                                }
                            } else if (current_state > 0) {
                                // Moving down in menu
                                if (gui->menu_state == 0) {
                                    int num_menu_items = 10;
                                    int items_per_page = 5;
                                    gui->menu_selection = (gui->menu_selection + 1) % num_menu_items;
                                    int max_scroll = (num_menu_items > items_per_page) ? (num_menu_items - items_per_page) : 0;
                                    if (gui->menu_selection >= gui->main_menu_scroll_offset + items_per_page) {
                                        gui->main_menu_scroll_offset = gui->menu_selection - items_per_page + 1;
                                    }
                                    if (gui->main_menu_scroll_offset > max_scroll) {
                                        gui->main_menu_scroll_offset = max_scroll;
                                    }
                                } else if (gui->menu_state == 1) {
                                    menu_move_down(gui);
                                } else if (gui->menu_state == 3) {
                                    menu_move_down(gui);
                                } else if (gui->menu_state == 4) {
                                    menu_update_language(gui, 1);
                                } else if (gui->menu_state == 5 || gui->menu_state == 6) {
                                    menu_move_down(gui);
                                }
                            }
                            gui->last_axis_1_state = current_state;
                            gui->last_joystick_axis_time = current_time;
                        }
                    }
                } else {
                    // In-game joystick analog stick controls (no throttling needed for gameplay)
                    gui->visualizer.mouse_just_moved = false;  // Disable mouse following
                    
                    if (axis == 0) {
                        // Left stick X-axis
                        if (value < -AXIS_THRESHOLD) {
                            gui->visualizer.key_a_pressed = true;  // Turn left
                            gui->visualizer.key_d_pressed = false;
                        } else if (value > AXIS_THRESHOLD) {
                            gui->visualizer.key_d_pressed = true;  // Turn right
                            gui->visualizer.key_a_pressed = false;
                        } else {
                            gui->visualizer.key_a_pressed = false;
                            gui->visualizer.key_d_pressed = false;
                        }
                    } else if (axis == 1) {
                        // Left stick Y-axis (inverted - down is positive)
                        if (value < -AXIS_THRESHOLD) {
                            gui->visualizer.key_w_pressed = true;   // Forward
                            gui->visualizer.key_s_pressed = false;
                        } else if (value > AXIS_THRESHOLD) {
                            gui->visualizer.key_s_pressed = true;   // Backward
                            gui->visualizer.key_w_pressed = false;
                        } else {
                            gui->visualizer.key_w_pressed = false;
                            gui->visualizer.key_s_pressed = false;
                        }
                    } else if (axis == 2 || axis == 5) {
                        // Right stick or triggers - could be used for aiming/weapons
                        // axis 2 = right stick X, axis 5 = right stick Y
                        // Currently not mapped, can be extended for right-stick aiming
                    }
                }
                break;
            }
            
            // HAT SWITCH (D-Pad) EVENTS
            case SDL_JOYHATMOTION: {
                int hat = event.jhat.value;
                
                // Handle cheat menu with D-Pad (takes priority)
                if (cheat_menu && cheat_menu->state == CHEAT_MENU_OPEN) {
                    if (hat & SDL_HAT_UP) {
                        cheat_menu->selection = (cheat_menu->selection - 1 + 7) % 7;
                        SDL_Log("[Comet Busters] [CHEAT] Selection up: %d\n", cheat_menu->selection);
                    } else if (hat & SDL_HAT_DOWN) {
                        cheat_menu->selection = (cheat_menu->selection + 1) % 7;
                        SDL_Log("[Comet Busters] [CHEAT] Selection down: %d\n", cheat_menu->selection);
                    } else if (hat & SDL_HAT_LEFT) {
                        switch (cheat_menu->selection) {
                            case 0: cheat_menu->wave = (cheat_menu->wave - 1 < 1) ? 30 : cheat_menu->wave - 1; break;
                            case 1: cheat_menu->lives = (cheat_menu->lives - 1 < 1) ? 20 : cheat_menu->lives - 1; break;
                            case 2: cheat_menu->missiles = (cheat_menu->missiles - 1 < 0) ? 99 : cheat_menu->missiles - 1; break;
                            case 3: cheat_menu->bombs = (cheat_menu->bombs - 1 < 0) ? 99 : cheat_menu->bombs - 1; break;
                            case 4: cheat_menu->cheat_difficulty = (cheat_menu->cheat_difficulty - 1 < 0) ? 2 : cheat_menu->cheat_difficulty - 1; break;
                        }
                        SDL_Log("[Comet Busters] [CHEAT] Value left\n");
                    } else if (hat & SDL_HAT_RIGHT) {
                        switch (cheat_menu->selection) {
                            case 0: cheat_menu->wave = (cheat_menu->wave + 1 > 30) ? 1 : cheat_menu->wave + 1; break;
                            case 1: cheat_menu->lives = (cheat_menu->lives + 1 > 20) ? 1 : cheat_menu->lives + 1; break;
                            case 2: cheat_menu->missiles = (cheat_menu->missiles + 1 > 99) ? 0 : cheat_menu->missiles + 1; break;
                            case 3: cheat_menu->bombs = (cheat_menu->bombs + 1 > 99) ? 0 : cheat_menu->bombs + 1; break;
                            case 4: cheat_menu->cheat_difficulty = (cheat_menu->cheat_difficulty + 1 > 2) ? 0 : cheat_menu->cheat_difficulty + 1; break;
                        }
                        SDL_Log("[Comet Busters] [CHEAT] Value right\n");
                    }
                    break;  // Don't process other D-Pad input while cheat menu is open
                }
                
                // Handle splash screen exit with D-Pad
                if (gui->visualizer.comet_buster.splash_screen_active && hat != SDL_HAT_CENTERED) {
                    SDL_Log("[Comet Busters] [SPLASH] D-Pad pressed (hat=%d) while splash active - exiting splash screen\n", hat);
                    
                    // Stop the intro music
                    SDL_Log("[Comet Busters] [SPLASH] Calling audio_stop_music()...\n");
                    audio_stop_music(&gui->audio);
                    
                    // Exit the splash screen
                    gui->visualizer.comet_buster.splash_screen_active = false;
                    
                    // Clear the board completely
                    gui->visualizer.comet_buster.comet_count = 0;
                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                    gui->visualizer.comet_buster.bullet_count = 0;
                    gui->visualizer.comet_buster.particle_count = 0;
                    gui->visualizer.comet_buster.floating_text_count = 0;
                    gui->visualizer.comet_buster.canister_count = 0;
                    gui->visualizer.comet_buster.missile_count = 0;
                    gui->visualizer.comet_buster.missile_pickup_count = 0;
                    gui->visualizer.comet_buster.bomb_count = 0;
                    gui->visualizer.comet_buster.bomb_pickup_count = 0;
                    gui->visualizer.comet_buster.score = 0;
                    gui->visualizer.comet_buster.score_multiplier = 1.0;
                    gui->visualizer.comet_buster.game_over = false;
                    gui->visualizer.comet_buster.game_won = false;
                    
                    // Spawn wave 1
#ifndef ANDROID
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
#else
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 720, 480);
#endif                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    SDL_Log("[Comet Busters] [SPLASH] Calling audio_play_random_music()...\n");
                    audio_play_random_music(&gui->audio);
                    SDL_Log("[Comet Busters] [SPLASH] Started gameplay music\n");
#endif
                    SDL_Log("[Comet Busters] [SPLASH] D-Pad splash exit complete\n");
                    break;  // Don't process other input while exiting splash
                }
                
                // Handle high score entry navigation with D-Pad
                if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE) {
                    int count = 0;
                    get_keyboard_buttons(&count);
                    int row_starts[] = {0, 10, 19, 26, 30};
                    int row_sizes[] = {10, 9, 7, 4};
                    
                    if (hat & SDL_HAT_UP) {
                        // Move up one row
                        int row = 0;
                        for (int r = 0; r < 4; r++) {
                            if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                row = r;
                                break;
                            }
                        }
                        if (row > 0) {
                            int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                            int new_row = row - 1;
                            if (pos_in_row >= row_sizes[new_row]) {
                                pos_in_row = row_sizes[new_row] - 1;
                            }
                            hs_entry->kb_selected_index = row_starts[new_row] + pos_in_row;
                        }
                    } else if (hat & SDL_HAT_DOWN) {
                        // Move down one row
                        int row = 0;
                        for (int r = 0; r < 4; r++) {
                            if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                row = r;
                                break;
                            }
                        }
                        if (row < 3) {
                            int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                            int new_row = row + 1;
                            if (pos_in_row >= row_sizes[new_row]) {
                                pos_in_row = row_sizes[new_row] - 1;
                            }
                            hs_entry->kb_selected_index = row_starts[new_row] + pos_in_row;
                        }
                    } else if (hat & SDL_HAT_LEFT) {
                        // Move left within row
                        int row = 0;
                        for (int r = 0; r < 4; r++) {
                            if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                row = r;
                                break;
                            }
                        }
                        int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                        if (pos_in_row > 0) {
                            hs_entry->kb_selected_index--;
                        } else {
                            hs_entry->kb_selected_index = row_starts[row] + row_sizes[row] - 1;
                        }
                    } else if (hat & SDL_HAT_RIGHT) {
                        // Move right within row
                        int row = 0;
                        for (int r = 0; r < 4; r++) {
                            if (hs_entry->kb_selected_index < row_starts[r+1]) {
                                row = r;
                                break;
                            }
                        }
                        int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                        if (pos_in_row < row_sizes[row] - 1) {
                            hs_entry->kb_selected_index++;
                        } else {
                            hs_entry->kb_selected_index = row_starts[row];
                        }
                    }
                    break;  // Don't process other input while in high score entry
                }
                
                if (gui->show_menu) {
                    if (gui->menu_state == 0) {
                        if (hat & SDL_HAT_UP) {
                            menu_move_up(gui);
                        } else if (hat & SDL_HAT_DOWN) {
                            menu_move_down(gui);
                        }
                    } else if (gui->menu_state == 1) {
                        // Difficulty selection (2 items: Easy, Normal, Hard)
                        if (hat & SDL_HAT_UP) {
                            menu_move_up(gui);
                        } else if (hat & SDL_HAT_DOWN) {
                            menu_move_down(gui);
                        }
                    } else if (gui->menu_state == 2) {
                        // High scores display - no navigation needed (just display)
                    } else if (gui->menu_state == 3) {
                        // Audio menu (2-3 items: Music Volume, SFX Volume, Back)
                        if (hat & SDL_HAT_UP) {
                            menu_move_up(gui);
                        } else if (hat & SDL_HAT_DOWN) {
                            menu_move_down(gui);
                        } else if (hat & SDL_HAT_LEFT) {
                            menu_update_volume(gui, gui->menu_selection, -1);
                        } else if (hat & SDL_HAT_RIGHT) {
                            menu_update_volume(gui, gui->menu_selection, 1);
                        }
                    } else if (gui->menu_state == 4) {
                        if (hat & SDL_HAT_UP) {
                            menu_update_language(gui, -1);
                        } else if (hat & SDL_HAT_DOWN) {
                            menu_update_language(gui, 1);
                        }
                    } else if (gui->menu_state == 5 || gui->menu_state == 6) {
                        if (hat & SDL_HAT_UP) {
                            menu_move_up(gui);
                        } else if (hat & SDL_HAT_DOWN) {
                            menu_move_down(gui);
                        }
                    }
                }

                break;
            }
            
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                init_joystick(gui);
                SDL_Log("[Comet Busters] [JOYSTICK] Device change detected, re-initializing joystick\n");
                break;
        }
    }
}

void handle_keyboard_input_special(SDL_Event *event, CometGUI *gui) {
    if (!gui) return;
    
    switch (event->key.keysym.sym) {
        case SDLK_p: {
            // P key to pause - check three conditions
            // Only allow pause if game is not over AND splash screen is not active AND finale is not active
            if (!gui->visualizer.comet_buster.game_over && 
                !gui->visualizer.comet_buster.splash_screen_active &&
                !gui->visualizer.comet_buster.finale_splash_active) {
                gui->game_paused = !gui->game_paused;
                
                // Handle music when pausing/resuming
                if (gui->game_paused) {
                    audio_stop_music(&gui->audio);
                    SDL_Log("[Comet Busters] [PAUSE] ========== GAME PAUSED ==========\n");
                    SDL_Log("[Comet Busters] [PAUSE] Wave: %d | Score: %d | Lives: %d\n",
                           gui->visualizer.comet_buster.current_wave,
                           gui->visualizer.comet_buster.score,
                           gui->visualizer.comet_buster.ship_lives);
                    SDL_Log("[Comet Busters] [PAUSE] Press P to resume or ESC for menu\n");
                    SDL_Log("[Comet Busters] [PAUSE] ====================================\n");
                } else {
                    SDL_Log("[Comet Busters] [RESUME] ========== GAME RESUMED ==========\n");
                    // Restart music if still in game (not in menu)
#ifdef ExternalSound
                    if (!gui->show_menu) {
                        audio_play_random_music(&gui->audio);
                    }
#endif
                    SDL_Log("[Comet Busters] [RESUME] ====================================\n");
                }
            }
            break;
        }
        case SDLK_c: {
            // CTRL+C to quit
            if ((event->key.keysym.mod & KMOD_CTRL)) {
                SDL_Log("[Comet Busters] [*] CTRL+C pressed - exiting\n");
                gui->running = false;
            }
            break;
        }
        case SDLK_k: {
            // CTRL+K to toggle cheat menu (for future use)
            if ((event->key.keysym.mod & KMOD_CTRL)) {
                SDL_Log("[Comet Busters] [*] CTRL+K pressed - cheat menu not yet implemented\n");
            }
            break;
        }
    }
}

// Check if mouse is over a keyboard button
int get_keyboard_button_at_pos(int mouse_x, int mouse_y) {
    int count = 0;
    KeyboardButton *buttons = get_keyboard_buttons(&count);
    
    for (int i = 0; i < count; i++) {
        if (mouse_x >= buttons[i].x && mouse_x <= buttons[i].x + buttons[i].width &&
            mouse_y >= buttons[i].y && mouse_y <= buttons[i].y + buttons[i].height) {
            return i;  // Found button
        }
    }
    return -1;  // No button at this position
}

// Add a character to the name input
void add_character_to_input(HighScoreEntryUI *hs_entry, char c) {
    if (hs_entry->cursor_pos < 31) {
        if (c == ' ') {
            hs_entry->name_input[hs_entry->cursor_pos++] = ' ';
        } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            hs_entry->name_input[hs_entry->cursor_pos++] = c;
        }
        hs_entry->name_input[hs_entry->cursor_pos] = '\0';
    }
}

void render_virtual_keyboard(CometGUI *gui, HighScoreEntryUI *hs_entry, int selected_index) {
    extern void gl_set_color(float r, float g, float b);
    extern void gl_draw_rect_filled(float x, float y, float width, float height);
    extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
    
    int count = 0;
    KeyboardButton *buttons = get_keyboard_buttons(&count);
    
    // Draw all keyboard buttons
    for (int i = 0; i < count; i++) {
        KeyboardButton *btn = &buttons[i];
        
        // Highlight selected button
        bool is_selected = (i == selected_index);
        
        if (is_selected) {
            // Yellow highlight for selected - thick border
            gl_set_color(1.0f, 1.0f, 0.0f);
            gl_draw_rect_filled(btn->x - 3, btn->y - 3, btn->width + 6, btn->height + 6);
        }
        
        // Button background color
        if (btn->is_special) {
            // Special buttons (red) - better contrast
            gl_set_color(0.9f, 0.2f, 0.2f);
        } else {
            // Regular letter buttons (darker blue for contrast)
            gl_set_color(0.0f, 0.4f, 0.8f);
        }
        gl_draw_rect_filled(btn->x, btn->y, btn->width, btn->height);
        
        // Button border - darker gold
        gl_set_color(0.6f, 0.4f, 0.1f);
        gl_draw_rect_filled(btn->x - 2, btn->y - 2, btn->width + 4, btn->height + 4);
        
        // Button text - pure white for maximum contrast
        gl_set_color(1.0f, 1.0f, 1.0f);
        
        if (btn->label != NULL) {
            // Special button - use label
            gl_draw_text_simple(btn->label, btn->x + 10, btn->y + 8, 9);
        } else if (btn->character == ' ') {
            gl_draw_text_simple(key_space[gui->visualizer.comet_buster.current_language], btn->x + 20, btn->y + 8, 9);
        } else if (btn->character != '\b' && btn->character != '\n') {
            // Regular letter - larger and bold-looking
            char text[2] = {btn->character, '\0'};
            gl_draw_text_simple(text, btn->x + 22, btn->y + 8, 14);
        }
    }
}

// ============================================================
// INITIALIZATION
// ============================================================

void init_joystick(CometGUI *gui) {
    gui->joystick = NULL;
    int num_joysticks = SDL_NumJoysticks();
    if (num_joysticks > 0) {
        gui->joystick = SDL_JoystickOpen(0);
        if (gui->joystick) {
            SDL_Log("[Comet Busters] [JOYSTICK] Found: %s\n", SDL_JoystickName(gui->joystick));
            SDL_Log("[Comet Busters] [JOYSTICK] Buttons: %d\n", SDL_JoystickNumButtons(gui->joystick));
            SDL_Log("[Comet Busters] [JOYSTICK] Axes: %d\n", SDL_JoystickNumAxes(gui->joystick));
            SDL_Log("[Comet Busters] [JOYSTICK] Hats: %d\n", SDL_JoystickNumHats(gui->joystick));
            SDL_Log("[Comet Busters] [JOYSTICK] ===== BUTTON MAPPING =====\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 0 (A/Cross)      - Fire/Select\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 1 (B/Circle)     - Boost/Back\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 2 (X/Square)     - Toggle Missiles\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 3 (Y/Triangle)   - Alt Fire\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 4 (LB/L1)        - Pause\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 5 (RB/R1)        - Cheat Menu\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Button 7 (Start)        - Toggle Menu\n");
            SDL_Log("[Comet Busters] [JOYSTICK] Left Stick X/Y          - Move/Rotate\n");
            SDL_Log("[Comet Busters] [JOYSTICK] D-Pad/Hat               - Menu Navigation\n");
            SDL_Log("[Comet Busters] [JOYSTICK] ============================\n");
        }
    } else {
        SDL_Log("[Comet Busters] [JOYSTICK] No joysticks detected\n");
    }
}

KeyboardButton* get_keyboard_buttons(int *out_count) {
    // Allocate space for all buttons (26 letters + space + backspace + clear + submit)
    static KeyboardButton buttons[31];
    int count = 0;
    
    // Standard QWERTY Layout:
    // Row 1: Q W E R T Y U I O P (10 keys)
    // Row 2: A S D F G H J K L (9 keys)
    // Row 3: Z X C V B N M (7 keys)
    // Row 4: SPACE BACK CLEAR DONE (4 keys)
    
    int start_x = 550;
    int start_y = 580;
    int btn_width = 60;
    int btn_height = 40;
    int spacing_x = 65;
    int spacing_y = 45;
    
    // Row 1: Q W E R T Y U I O P (10 keys)
    const char row1[] = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'};
    for (int i = 0; i < 10; i++) {
        buttons[count].character = row1[i];
        buttons[count].label = NULL;
        buttons[count].is_special = false;
        buttons[count].x = start_x + (i * spacing_x);
        buttons[count].y = start_y;
        buttons[count].width = btn_width;
        buttons[count].height = btn_height;
        count++;
    }
    
    // Row 2: A S D F G H J K L (9 keys, offset by 32 pixels)
    const char row2[] = {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'};
    for (int i = 0; i < 9; i++) {
        buttons[count].character = row2[i];
        buttons[count].label = NULL;
        buttons[count].is_special = false;
        buttons[count].x = start_x + 32 + (i * spacing_x);
        buttons[count].y = start_y + spacing_y;
        buttons[count].width = btn_width;
        buttons[count].height = btn_height;
        count++;
    }
    
    // Row 3: Z X C V B N M (7 keys, offset by 64 pixels)
    const char row3[] = {'Z', 'X', 'C', 'V', 'B', 'N', 'M'};
    for (int i = 0; i < 7; i++) {
        buttons[count].character = row3[i];
        buttons[count].label = NULL;
        buttons[count].is_special = false;
        buttons[count].x = start_x + 64 + (i * spacing_x);
        buttons[count].y = start_y + (2 * spacing_y);
        buttons[count].width = btn_width;
        buttons[count].height = btn_height;
        count++;
    }
    
    // Row 4: SPACE BACK CLEAR DONE
    
    // Space button (wider)
    buttons[count].character = ' ';
    buttons[count].label = "SPACE";
    buttons[count].is_special = false;
    buttons[count].x = start_x + 100;
    buttons[count].y = start_y + (3 * spacing_y);
    buttons[count].width = 120;  // Wider
    buttons[count].height = btn_height;
    count++;
    
    // Backspace button
    buttons[count].character = '\b';  // Backspace character
    buttons[count].label = "BACK";
    buttons[count].is_special = true;
    buttons[count].x = start_x + 240;
    buttons[count].y = start_y + (3 * spacing_y);
    buttons[count].width = btn_width;
    buttons[count].height = btn_height;
    count++;
    
    // Clear button
    buttons[count].character = 'C';
    buttons[count].label = "CLEAR";
    buttons[count].is_special = true;
    buttons[count].x = start_x + 240 + spacing_x;
    buttons[count].y = start_y + (3 * spacing_y);
    buttons[count].width = btn_width;
    buttons[count].height = btn_height;
    count++;
    
    // Submit button
    buttons[count].character = '\n';  // Newline for submit
    buttons[count].label = "DONE";
    buttons[count].is_special = true;
    buttons[count].x = start_x + 240 + (2 * spacing_x);
    buttons[count].y = start_y + (3 * spacing_y);
    buttons[count].width = btn_width;
    buttons[count].height = btn_height;
    count++;
    
    *out_count = count;
    return buttons;
}

void handle_keyboard_input(SDL_Event *event, CometGUI *gui, HighScoreEntryUI *hs_entry) {
    if (!gui) return;
    bool pressed = (event->type == SDL_KEYDOWN);
    
    // Handle high score name entry
    if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE && pressed) {
        switch (event->key.keysym.sym) {
            case SDLK_BACKSPACE: {
                // Delete last character
                if (hs_entry->cursor_pos > 0) {
                    hs_entry->cursor_pos--;
                    hs_entry->name_input[hs_entry->cursor_pos] = '\0';
                }
                break;
            }
            case SDLK_RETURN: {
                // Submit the name
                if (hs_entry->cursor_pos > 0) {
                    // Save the high score
                    high_scores_add(&gui->visualizer.comet_buster, 
                                   gui->visualizer.comet_buster.score,
                                   gui->visualizer.comet_buster.current_wave,
                                   hs_entry->name_input);
                    high_scores_save(&gui->visualizer.comet_buster);
                    
                    // Reset game to menu
                    hs_entry->state = HIGH_SCORE_ENTRY_SAVED;
                    gui->visualizer.comet_buster.game_over = false;
                    gui->show_menu = true;
                    gui->menu_state = 2;  // Show high scores
                    gui->menu_selection = 2;
                }
                break;
            }
            case SDLK_LEFT: {
                // Navigate virtual keyboard left
                int count = 0;
                get_keyboard_buttons(&count);
                
                // Determine row and position in row
                int row = 0;
                int row_starts[] = {0, 10, 19, 26, 30};  // Start index of each row
                int row_sizes[] = {10, 9, 7, 4};  // Size of each row
                
                for (int r = 0; r < 4; r++) {
                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                        row = r;
                        break;
                    }
                }
                
                int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                if (pos_in_row > 0) {
                    hs_entry->kb_selected_index--;
                } else {
                    // At start of row, go to end
                    hs_entry->kb_selected_index = row_starts[row] + row_sizes[row] - 1;
                }
                break;
            }
            case SDLK_RIGHT: {
                // Navigate virtual keyboard right
                int count = 0;
                get_keyboard_buttons(&count);
                
                // Determine row and position in row
                int row = 0;
                int row_starts[] = {0, 10, 19, 26, 30};
                int row_sizes[] = {10, 9, 7, 4};
                
                for (int r = 0; r < 4; r++) {
                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                        row = r;
                        break;
                    }
                }
                
                int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                if (pos_in_row < row_sizes[row] - 1) {
                    hs_entry->kb_selected_index++;
                } else {
                    // At end of row, go to start
                    hs_entry->kb_selected_index = row_starts[row];
                }
                break;
            }
            case SDLK_UP: {
                // Navigate virtual keyboard up
                int row = 0;
                int row_starts[] = {0, 10, 19, 26, 30};
                
                for (int r = 0; r < 4; r++) {
                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                        row = r;
                        break;
                    }
                }
                
                if (row > 0) {
                    // Move up one row, keep relative position
                    int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                    int new_row = row - 1;
                    int row_sizes[] = {10, 9, 7, 4};
                    
                    // Don't go past end of new row
                    if (pos_in_row >= row_sizes[new_row]) {
                        pos_in_row = row_sizes[new_row] - 1;
                    }
                    hs_entry->kb_selected_index = row_starts[new_row] + pos_in_row;
                }
                break;
            }
            case SDLK_DOWN: {
                // Navigate virtual keyboard down
                int row = 0;
                int row_starts[] = {0, 10, 19, 26, 30};
                
                for (int r = 0; r < 4; r++) {
                    if (hs_entry->kb_selected_index < row_starts[r+1]) {
                        row = r;
                        break;
                    }
                }
                
                if (row < 3) {
                    // Move down one row, keep relative position
                    int pos_in_row = hs_entry->kb_selected_index - row_starts[row];
                    int new_row = row + 1;
                    int row_sizes[] = {10, 9, 7, 4};
                    
                    // Don't go past end of new row
                    if (pos_in_row >= row_sizes[new_row]) {
                        pos_in_row = row_sizes[new_row] - 1;
                    }
                    hs_entry->kb_selected_index = row_starts[new_row] + pos_in_row;
                }
                break;
            }
            default: {
                if (hs_entry->cursor_pos < 31) {
                    SDL_Keycode key = event->key.keysym.sym;
                    SDL_Keymod mod  = (SDL_Keymod) event->key.keysym.mod;

                    bool shift = (mod & KMOD_SHIFT) != 0;

                    // Letters only
                    if (key >= SDLK_a && key <= SDLK_z) {
                        char c = (char)key;

                        if (shift)
                            c = c - 'a' + 'A';   // uppercase
                        else
                            c = c;               // lowercase

                        hs_entry->name_input[hs_entry->cursor_pos++] = c;
                        hs_entry->name_input[hs_entry->cursor_pos] = '\0';
                    }
                    else if (key == SDLK_SPACE) {
                        hs_entry->name_input[hs_entry->cursor_pos++] = ' ';
                        hs_entry->name_input[hs_entry->cursor_pos] = '\0';
                    }
                }
                break;
            }
        }
        return;  // Don't process other input while in name entry
    }
    
    switch (event->key.keysym.sym) {
        // Movement keys - A/D/W/S and Arrow keys
        case SDLK_a:
        case SDLK_LEFT:
            gui->visualizer.key_a_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        case SDLK_d:
        case SDLK_RIGHT:
            gui->visualizer.key_d_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        case SDLK_w:
        case SDLK_UP:
            gui->visualizer.key_w_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        case SDLK_s:
        case SDLK_DOWN:
            gui->visualizer.key_s_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        
        // Fire keys
        case SDLK_z:
            gui->visualizer.key_z_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        case SDLK_SPACE:
        case SDLK_x:
            gui->visualizer.key_x_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            gui->visualizer.key_ctrl_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        
        // Special keys
        case SDLK_q:
            gui->visualizer.key_q_pressed = pressed;
            gui->visualizer.mouse_just_moved = false;
            break;
        case SDLK_F5:
            gui->visualizer.mouse_just_moved = false;
            if (pressed) {
                menu_save_state(gui, 10);
            }
            break;
        
        case SDLK_F7:
            gui->visualizer.mouse_just_moved = false;
            if (pressed) {
                menu_load_state(gui, 10);
                SDL_Log("[Comet Busters] [INPUT] F7 - Quick loaded from slot 10\n");
            }
            break;
        
        case SDLK_F11:
            gui->visualizer.mouse_just_moved = false;
            if (pressed) {
                gui->fullscreen = !gui->fullscreen;
                if (gui->fullscreen) {
                    SDL_SetWindowFullscreen(gui->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                } else {
                    SDL_SetWindowFullscreen(gui->window, 0);
                }
                SDL_Log("[Comet Busters] [INPUT] F11 - Fullscreen toggled: %s\n", gui->fullscreen ? "ON" : "OFF");
            }
            break;
    }
}
