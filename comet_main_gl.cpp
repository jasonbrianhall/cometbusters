#include <SDL2/SDL.h>
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

// ============================================================
// LOCAL HIGH SCORE ENTRY STATE (Not in header - local only)
// ============================================================

typedef enum {
    HIGH_SCORE_ENTRY_NONE = 0,
    HIGH_SCORE_ENTRY_ACTIVE = 1,
    HIGH_SCORE_ENTRY_SAVED = 2
} HighScoreEntryState;

typedef struct {
    int state;                  // HighScoreEntryState
    char name_input[32];        // Player's typed name
    int cursor_pos;             // Current cursor position
} HighScoreEntryUI;

// ============================================================
// LOCAL CHEAT MENU STATE
// ============================================================

typedef enum {
    CHEAT_MENU_CLOSED = 0,
    CHEAT_MENU_OPEN = 1
} CheatMenuState;

typedef struct {
    int state;              // CHEAT_MENU_OPEN or CHEAT_MENU_CLOSED
    int selection;          // 0=Wave, 1=Lives, 2=Missiles, 3=Bombs, 4=Apply, 5=Cancel
    int wave;               // Selected wave (1-30)
    int lives;              // Selected lives (1-20)
    int missiles;           // Selected missiles
    int bombs;              // Selected bombs
    int cheat_difficulty;   // Cheat Difficulty
} CheatMenuUI;

#ifdef _WIN32
std::string getExecutableDir() { 
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : path.substr(0, pos);
}
#else
std::string getExecutableDir() {
    return ".";
}
#endif

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    Visualizer visualizer;
    AudioManager audio;
    
    int window_width;
    int window_height;
    bool fullscreen;
    bool running;
    bool game_paused;
    
    // Menu state
    bool show_menu;
    int menu_selection;  // 0=Continue, 1=New Game, 2=High Scores, 3=Audio, 4=Quit
    int menu_state;      // 0=Main Menu, 1=Difficulty Select, 2=High Scores Display, 3=Audio Menu
    int gui_difficulty_level; // 1-3
    
    // Music/Audio state tracking
    bool finale_music_started;  // Tracks if finale music has been played
    
    int frame_count;
    double total_time;
    double delta_time;
    uint32_t last_frame_ticks;
    
    SDL_Joystick *joystick;
    int music_volume;
    int sfx_volume;
} CometGUI;

// ============================================================
// INPUT HANDLING
// ============================================================

static void handle_keyboard_input(SDL_Event *event, CometGUI *gui, HighScoreEntryUI *hs_entry) {
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
            default: {
                // Add character (only letters A-Z)
                if (hs_entry->cursor_pos < 31) {
                    char c = event->key.keysym.sym;
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A'; // Convert to uppercase
                        hs_entry->name_input[hs_entry->cursor_pos] = c;
                        hs_entry->cursor_pos++;
                        hs_entry->name_input[hs_entry->cursor_pos] = '\0';
                    } else if (c == SDLK_SPACE) {
                        // Allow spaces too
                        hs_entry->name_input[hs_entry->cursor_pos] = ' ';
                        hs_entry->cursor_pos++;
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
        case SDLK_F11:
            gui->visualizer.mouse_just_moved = false;
            if (pressed) {
                gui->fullscreen = !gui->fullscreen;
                if (gui->fullscreen) {
                    SDL_SetWindowFullscreen(gui->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                } else {
                    SDL_SetWindowFullscreen(gui->window, 0);
                }
                printf("[INPUT] F11 - Fullscreen toggled: %s\n", gui->fullscreen ? "ON" : "OFF");
            }
            break;
    }
}

static void handle_keyboard_input_special(SDL_Event *event, CometGUI *gui) {
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
                    printf("[PAUSE] ========== GAME PAUSED ==========\n");
                    printf("[PAUSE] Wave: %d | Score: %d | Lives: %d\n",
                           gui->visualizer.comet_buster.current_wave,
                           gui->visualizer.comet_buster.score,
                           gui->visualizer.comet_buster.ship_lives);
                    printf("[PAUSE] Press P to resume or ESC for menu\n");
                    printf("[PAUSE] ====================================\n");
                } else {
                    printf("[RESUME] ========== GAME RESUMED ==========\n");
                    // Restart music if still in game (not in menu)
#ifdef ExternalSound
                    if (!gui->show_menu) {
                        audio_play_random_music(&gui->audio);
                    }
#endif
                    printf("[RESUME] ====================================\n");
                }
            }
            break;
        }
        case SDLK_c: {
            // CTRL+C to quit
            if ((event->key.keysym.mod & KMOD_CTRL)) {
                printf("[*] CTRL+C pressed - exiting\n");
                gui->running = false;
            }
            break;
        }
        case SDLK_k: {
            // CTRL+K to toggle cheat menu (for future use)
            if ((event->key.keysym.mod & KMOD_CTRL)) {
                printf("[*] CTRL+K pressed - cheat menu not yet implemented\n");
            }
            break;
        }
    }
}

// ============================================================
// INITIALIZATION
// ============================================================

static void init_joystick(CometGUI *gui) {
    gui->joystick = NULL;
    int num_joysticks = SDL_NumJoysticks();
    if (num_joysticks > 0) {
        gui->joystick = SDL_JoystickOpen(0);
        if (gui->joystick) {
            printf("[JOYSTICK] Found: %s\n", SDL_JoystickName(gui->joystick));
        }
    }
}

static bool init_sdl_and_opengl(CometGUI *gui, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "[ERROR] SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    
    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE;
    if (gui->fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    
    gui->window = SDL_CreateWindow(
        "Comet Busters",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        flags
    );
    
    if (!gui->window) {
        fprintf(stderr, "[ERROR] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // Ensure window is visible on all platforms
    SDL_ShowWindow(gui->window);
    SDL_RaiseWindow(gui->window);
    
    // Get the actual window size (might be different if maximized)
    SDL_GetWindowSize(gui->window, &gui->window_width, &gui->window_height);
    printf("[SDL] Window created: %dx%d\n", gui->window_width, gui->window_height);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    gui->gl_context = SDL_GL_CreateContext(gui->window);
    if (!gui->gl_context) {
        fprintf(stderr, "[ERROR] GL context failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        return false;
    }
    
    SDL_GL_SetSwapInterval(1);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "[ERROR] GLEW init failed\n");
        SDL_GL_DeleteContext(gui->gl_context);
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        return false;
    }
    
    printf("[INIT] SDL2, OpenGL %s, GLEW OK\n", glGetString(GL_VERSION));
    
    // Set background to match Cairo version dark blue
    glClearColor(0.05f, 0.075f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    init_joystick(gui);
    return true;
}

// ============================================================
// GAME LOOP
// ============================================================

static void handle_events(CometGUI *gui, HighScoreEntryUI *hs_entry, CheatMenuUI *cheat_menu) {
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
                    printf("[WINDOW] Resized to %dx%d\n", gui->window_width, gui->window_height);
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
                    fprintf(stdout, "[SPLASH] User pressed key - exiting splash screen\n");
                    
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
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    audio_play_random_music(&gui->audio);
                    fprintf(stdout, "[SPLASH] Started gameplay music\n");
#endif
                    break;  // Skip other input processing when exiting splash
                }
                
                // Handle escape for menu toggle
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (gui->show_menu && gui->menu_state != 0) {
                        gui->menu_state = 0;
                        gui->menu_selection = 1;
                    } else {
                        gui->show_menu = !gui->show_menu;
                        gui->menu_state = 0;
                        gui->menu_selection = 0;
                    }
                    break;
                }
                // Handle cheat menu and menu input
                if (gui->show_menu) {
                    // Handle cheat menu input first (takes priority)
                    if (cheat_menu && cheat_menu->state == CHEAT_MENU_OPEN) {
                        if (event.key.keysym.sym == SDLK_UP) {
                            cheat_menu->selection = (cheat_menu->selection - 1 + 7) % 7;
                        } else if (event.key.keysym.sym == SDLK_DOWN) {
                            cheat_menu->selection = (cheat_menu->selection + 1) % 7;
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
                                    printf("[CHEAT] Wave changed from %d to %d - spawning new wave\n", old_wave, new_wave);
                                    
                                    // Clear all entities
                                    gui->visualizer.comet_buster.comet_count = 0;
                                    gui->visualizer.comet_buster.enemy_ship_count = 0;
                                    gui->visualizer.comet_buster.enemy_bullet_count = 0;
                                    gui->visualizer.comet_buster.bullet_count = 0;
                                    gui->visualizer.comet_buster.particle_count = 0;
                                    gui->visualizer.comet_buster.missile_count = 0;
                                    
                                    // Spawn the new wave
                                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
                                    printf("[CHEAT] Spawned Wave %d\n", new_wave);
                                } else {
                                    printf("[CHEAT] Wave unchanged (still Wave %d) - just updated Lives/Missiles/Bombs\n", new_wave);
                                }
                                
                                printf("[CHEAT] Applied: Wave=%d, Lives=%d, Missiles=%d, Bombs=%d%s\n",
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
                        printf("[CHEAT] Error: cheat_menu is NULL\n");
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
                            printf("[CHEAT] Opening cheat menu (Current: Wave=%d, Lives=%d, Missiles=%d, Bombs=%d)\n",
                                   cheat_menu->wave, cheat_menu->lives, cheat_menu->missiles, cheat_menu->bombs);
                        }
                        break;
                    }
                    
                    // Handle regular menu navigation
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection - 1 + 5) % 5;
                        } else if (gui->menu_state == 1) {
                            gui->gui_difficulty_level = (gui->gui_difficulty_level - 1);
                            if (gui->gui_difficulty_level < 1) gui->gui_difficulty_level = 3;
                        } else if (gui->menu_state == 3) {
                            gui->menu_selection = (gui->menu_selection - 1 + 2) % 2;
                        }
                    } else if (event.key.keysym.sym == SDLK_DOWN) {
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection + 1) % 5;
                        } else if (gui->menu_state == 1) {
                            gui->gui_difficulty_level = (gui->gui_difficulty_level + 1);
                            if (gui->gui_difficulty_level > 3) gui->gui_difficulty_level = 1;
                        } else if (gui->menu_state == 3) {
                            gui->menu_selection = (gui->menu_selection + 1) % 2;
                        }
                    } else if (event.key.keysym.sym == SDLK_LEFT) {
                        if (gui->menu_state == 3) {
                            if (gui->menu_selection == 0) {
                                gui->music_volume = (gui->music_volume - 5);
                                if (gui->music_volume < 0) gui->music_volume = 0;
                                audio_set_music_volume(&gui->audio, gui->music_volume);
                            } else {
                                gui->sfx_volume = (gui->sfx_volume - 5);
                                if (gui->sfx_volume < 0) gui->sfx_volume = 0;
                                audio_set_sfx_volume(&gui->audio, gui->sfx_volume);
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_RIGHT) {
                        if (gui->menu_state == 3) {
                            if (gui->menu_selection == 0) {
                                gui->music_volume = (gui->music_volume + 5);
                                if (gui->music_volume > 100) gui->music_volume = 100;
                                audio_set_music_volume(&gui->audio, gui->music_volume);
                            } else {
                                gui->sfx_volume = (gui->sfx_volume + 5);
                                if (gui->sfx_volume > 100) gui->sfx_volume = 100;
                                audio_set_sfx_volume(&gui->audio, gui->sfx_volume);
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_c) {
                        // Open cheat menu from main menu
                        if (gui->show_menu && gui->menu_state == 0) {
                            cheat_menu->state = CHEAT_MENU_OPEN;
                            cheat_menu->selection = 0;
                            printf("[CHEAT] Opening cheat menu\n");
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_RETURN) {
                        if (gui->menu_state == 0) {
                            switch (gui->menu_selection) {
                                case 0:  // Continue
                                    gui->show_menu = false;
                                    break;
                                case 1:  // New Game - go to difficulty selection
                                    gui->menu_state = 1;
                                    gui->gui_difficulty_level = 1;
                                    break;
                                case 2:  // High Scores
                                    gui->menu_state = 2;
                                    break;
                                case 3:  // Audio
                                    gui->menu_state = 3;
                                    gui->menu_selection = 0;
                                    break;
                                case 4:  // Quit
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
                            audio_play_intro_music(&gui->audio, "music/intro.mp3");
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
                if (gui->visualizer.comet_buster.splash_screen_active) {
                    fprintf(stdout, "[SPLASH] User clicked - exiting splash screen\n");
                    
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
                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
                    
                    // Start gameplay music rotation
#ifdef ExternalSound
                    audio_play_random_music(&gui->audio);
                    fprintf(stdout, "[SPLASH] Started gameplay music\n");
#endif
                    break;
                }
                
                if (gui->show_menu) {
                    // Convert from window coordinates to game space (1920x1080)
                    // This matches how render_frame uses glOrtho for scaling
                    int mouse_x = (int)((event.button.x / (float)gui->window_width) * 1920.0f);
                    int mouse_y = (int)((event.button.y / (float)gui->window_height) * 1080.0f);
                    
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // Main menu buttons
                        if (gui->menu_state == 0) {
                            const int menu_y_start = 350;
                            const int menu_spacing = 120;
                            const int menu_width = 400;
                            const int menu_height = 60;
                            
                            for (int i = 0; i < 5; i++) {
                                int menu_y = menu_y_start + (i * menu_spacing);
                                int menu_x = (1920 - menu_width) / 2;
                                
                                if (mouse_x >= menu_x && mouse_x <= menu_x + menu_width &&
                                    mouse_y >= menu_y && mouse_y <= menu_y + menu_height) {
                                    gui->menu_selection = i;
                                    switch (gui->menu_selection) {
                                        case 0:  // Continue
                                            gui->show_menu = false;
                                            break;
                                        case 1:  // New Game
                                            gui->menu_state = 1;
                                            gui->gui_difficulty_level = 1;
                                            break;
                                        case 2:  // High Scores
                                            gui->menu_state = 2;
                                            break;
                                        case 3:  // Audio
                                            gui->menu_state = 3;
                                            gui->menu_selection = 0;
                                            break;
                                        case 4:  // Quit
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
                            const int diff_x = (1920 - diff_width) / 2;
                            
                            for (int i = 0; i < 3; i++) {
                                int diff_y = diff_y_start + (i * diff_spacing);
                                if (mouse_x >= diff_x && mouse_x <= diff_x + diff_width &&
                                    mouse_y >= diff_y && mouse_y <= diff_y + diff_height) {
                                    gui->gui_difficulty_level = i + 1;
                                    // Start game with selected difficulty
                                    comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, gui->gui_difficulty_level-1);
#ifdef ExternalSound
                                    audio_play_intro_music(&gui->audio, "music/intro.mp3");
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
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        gui->visualizer.mouse_right_pressed = true;
                        
                        // Right-click to restart if game over
                        if (gui->visualizer.comet_buster.game_over) {
                            comet_buster_reset_game_with_splash(&gui->visualizer.comet_buster, true, gui->gui_difficulty_level-1);
                            gui->game_paused = false;
                            gui->finale_music_started = false;
#ifdef ExternalSound
                            audio_play_intro_music(&gui->audio, "music/intro.mp3");
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
                gui->visualizer.mouse_x = (int)(event.motion.x * 1920.0f / gui->window_width);
                gui->visualizer.mouse_y = (int)(event.motion.y * 1080.0f / gui->window_height);

                break;
            }
            
            case SDL_MOUSEWHEEL:
                gui->visualizer.mouse_just_moved = true;
                gui->visualizer.scroll_direction = event.wheel.y;
                break;
            
/*            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                init_joystick(gui);
                break;*/
        }
    }
}

static void update_game(CometGUI *gui, HighScoreEntryUI *hs_entry) {
    // Don't update if menu is open or game is paused
    if (gui->show_menu || gui->game_paused) return;
    
    // Handle finale splash if active (Wave 30 victory)
    // UPDATE THIS FIRST before skipping normal game update
    if (gui->visualizer.comet_buster.finale_splash_active) {
        // Start finale music on first frame of finale splash
        if (!gui->finale_music_started) {
            fprintf(stdout, "[FINALE] Starting finale music...\n");
            audio_stop_music(&gui->audio);
#ifdef ExternalSound
            audio_play_music(&gui->audio, "music/finale.mp3", false);  // Don't loop
#endif
            gui->finale_music_started = true;
        }
        
        // Update the finale splash (THIS IS CRITICAL - animates the finale)
        comet_buster_update_finale_splash(&gui->visualizer.comet_buster, 1.0 / 60.0);
        
        // Check if user wants to continue to next wave (can right-click anytime to skip)
        if (gui->visualizer.mouse_right_pressed) {
            fprintf(stdout, "[FINALE] Player skipping to Wave 31\n");
            
            // If scroll isn't done yet, fast-forward to the end
            if (!gui->visualizer.comet_buster.finale_waiting_for_input) {
                gui->visualizer.comet_buster.finale_scroll_line_index = 999;  // Jump to end of scroll
                gui->visualizer.comet_buster.finale_waiting_for_input = true;
            }
            
            // Stop the finale music
            audio_stop_music(&gui->audio);
            
            // Clean up finale splash
            gui->visualizer.comet_buster.finale_splash_active = false;
            gui->visualizer.comet_buster.finale_splash_boss_paused = false;
            gui->visualizer.comet_buster.boss.active = false;
            gui->visualizer.comet_buster.boss_active = false;
            
            // Advance to next wave
            gui->visualizer.comet_buster.current_wave++;
            comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
            
            // Reset flags and input
            gui->finale_music_started = false;
            gui->visualizer.mouse_right_pressed = false;
            
            // Start gameplay music rotation
#ifdef ExternalSound
            audio_play_random_music(&gui->audio);
#endif
        }
        
        // Skip normal game update during finale - freeze the background
        return;
    }
    
    // Call the master update function from visualization.h
    // This handles ALL game updates including collisions, audio, wave progression, etc.
    // (This is skipped during finale when we return above)
    update_comet_buster(&gui->visualizer, gui->delta_time);
    
    // Check if current music track has finished and queue the next one
#ifdef ExternalSound
    if (!gui->game_paused && !audio_is_music_playing(&gui->audio)) {
        fprintf(stdout, "[AUDIO] Current track finished, queuing next track...\n");
        audio_play_random_music(&gui->audio);
    }
#endif
    
    // Stop music if game ends and trigger high score entry
    if (gui->visualizer.comet_buster.game_over) {
        printf("\n[HS_FLOW] >>> GAME OVER DETECTED\n");
        audio_stop_music(&gui->audio);
        
        // Trigger high score entry if not already showing the dialog
        if (!hs_entry) {
            printf("[HS_FLOW] ERROR: hs_entry is NULL!\n");
            return;
        }
        
        printf("[HS_FLOW] hs_entry->state = %d (0=NONE, 1=ACTIVE, 2=SAVED)\n", hs_entry->state);
        
        if (hs_entry->state != HIGH_SCORE_ENTRY_ACTIVE) {
            int score = gui->visualizer.comet_buster.score;
            int count = gui->visualizer.comet_buster.high_score_count;
            
            printf("[HS_FLOW] Checking high score eligibility...\n");
            printf("[HS_FLOW] score=%d, high_score_count=%d, MAX=%d\n", 
                   score, count, 25);
            
            // Check if this is a high score
            bool is_high_score = comet_buster_is_high_score(&gui->visualizer.comet_buster, score);
            printf("[HS_FLOW] is_high_score() returned: %s\n", is_high_score ? "TRUE" : "FALSE");
            
            if (is_high_score) {
                printf("[HS_FLOW] >>> SHOWING DIALOG\n");
                // Show high score entry dialog
                hs_entry->state = HIGH_SCORE_ENTRY_ACTIVE;
                hs_entry->cursor_pos = 0;
                memset(hs_entry->name_input, 0, sizeof(hs_entry->name_input));
                printf("[HIGHSCORE] New high score! Score: %d\n", score);
            } else {
                printf("[HS_FLOW] >>> SHOWING MENU (score doesn't qualify)\n");
                // Score doesn't qualify - always go to menu
                gui->show_menu = true;
                gui->menu_state = 0;  // Main menu
                gui->menu_selection = 0;
                // Reset state so dialog can show again if next score qualifies
                hs_entry->state = HIGH_SCORE_ENTRY_NONE;
                printf("[HIGHSCORE] Game over. Score: %d (not a high score)\n", score);
            }
        } else {
            printf("[HS_FLOW] Dialog already active, not triggering again\n");
        }
        printf("[HS_FLOW] <<< END GAME OVER HANDLING\n\n");
    }
}

static void render_frame(CometGUI *gui, HighScoreEntryUI *hs_entry, CheatMenuUI *cheat_menu) {
    glClearColor(0.05f, 0.075f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    const float GAME_WIDTH = 1920.0f;
    const float GAME_HEIGHT = 1080.0f;
    
    // Fill entire window
    glViewport(0, 0, gui->window_width, gui->window_height);
    
    // Set projection matrix to scale game space to fill viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, GAME_WIDTH, GAME_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Game always in logical space
    gui->visualizer.width = 1920;
    gui->visualizer.height = 1080;
    
    // Draw game
    draw_comet_buster_gl(&gui->visualizer, NULL);
    
    // Render menu overlay if open
    if (gui->show_menu) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        // Main menu
        if (gui->menu_state == 0) {
            gl_set_color(0.0f, 1.0f, 1.0f);
            gl_draw_text_simple("COMET BUSTERS", 800, 100, 28);
            
            const char *menu_items[] = {
                "CONTINUE",
                "NEW GAME",
                "HIGH SCORES",
                "AUDIO",
                "QUIT"
            };
            
            int menu_y_start = 350;
            int menu_spacing = 120;
            int menu_width = 400;
            int menu_height = 60;
            
            for (int i = 0; i < 5; i++) {
                int menu_y = menu_y_start + (i * menu_spacing);
                int menu_x = (1920 - menu_width) / 2;
                
                if (i == gui->menu_selection) {
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
            
            gl_set_color(0.8f, 0.8f, 0.8f);
            gl_draw_text_simple("Up/Down/Enter to select; ESC to close", 550, 950, 24);
            
        } else if (gui->menu_state == 1) {
            // Difficulty selection menu
            gl_set_color(0.0f, 1.0f, 1.0f);
            gl_draw_text_simple("SELECT DIFFICULTY", 780, 150, 24);
            
            const char *difficulties[] = {
                "EASY",
                "NORMAL",
                "HARD"
            };
            
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
            gl_draw_text_simple("UP/DOWN to select | ENTER to start | ESC to go back", 550, 950, 10);
            
        } else if (gui->menu_state == 2) {
            // High scores display
            gl_set_color(0.0f, 1.0f, 1.0f);
            gl_draw_text_simple("HIGH SCORES", 850, 100, 20);
            
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
            gl_draw_text_simple("ENTER to continue | ESC to go back", 700, 950, 10);
            
        } else if (gui->menu_state == 3) {
            // Audio menu
            gl_set_color(0.0f, 1.0f, 1.0f);
            gl_draw_text_simple("AUDIO SETTINGS", 820, 150, 18);
            
            const char *audio_options[] = {
                "MUSIC VOLUME",
                "SFX VOLUME"
            };
            
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
            gl_draw_text_simple("UP/DOWN to select | LEFT/RIGHT to adjust | ESC to go back", 550, 950, 10);
        }
    }
    
    // Render high score entry dialog if active
    if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        // Dialog box background - Warm parchment (#F7F3E8)
        int dialog_x = 560;
        int dialog_y = 320;
        int dialog_width = 800;
        int dialog_height = 420;
        
        // Warm parchment background
        gl_set_color(0.968f, 0.953f, 0.91f);  // #F7F3E8
        gl_draw_rect_filled(dialog_x, dialog_y, dialog_width, dialog_height);
        
        // Warm gold/brown border
        gl_set_color(0.8f, 0.6f, 0.2f);  // Warm gold
        for (int i = 0; i < 3; i++) {
            gl_draw_rect_filled(dialog_x - i, dialog_y - i, dialog_width + 2*i, dialog_height + 2*i);
            if (i < 2) gl_set_color(0.8f, 0.6f, 0.2f);
        }
        
        // Title - Dark brown text
        gl_set_color(0.2f, 0.15f, 0.1f);  // Dark brown
        gl_draw_text_simple("NEW HIGH SCORE!", 740, 345, 24);
        
        // Score display - Dark brown
        gl_set_color(0.3f, 0.25f, 0.15f);
        char score_text[128];
        sprintf(score_text, "Score: %d | Wave: %d", 
                gui->visualizer.comet_buster.score,
                gui->visualizer.comet_buster.current_wave);
        gl_draw_text_simple(score_text, 620, 395, 14);
        
        // Name entry label - Dark brown
        gl_set_color(0.2f, 0.15f, 0.1f);
        gl_draw_text_simple("Enter Your Name:", 620, 445, 13);
        
        // Input box background - Slightly off-white
        gl_set_color(0.95f, 0.93f, 0.88f);
        gl_draw_rect_filled(620, 475, 660, 50);
        
        // Input box border - Warm gold
        gl_set_color(0.8f, 0.6f, 0.2f);
        gl_draw_rect_filled(618, 473, 664, 54);
        
        // Display the typed name - Dark brown
        gl_set_color(0.1f, 0.08f, 0.05f);  // Very dark brown
        gl_draw_text_simple(hs_entry->name_input, 635, 490, 16);
        
        // Cursor (blinking underscore)
        if ((int)(gui->total_time * 2) % 2 == 0) {  // Blink effect
            char cursor_text[] = "_";
            int cursor_x = 635 + (hs_entry->cursor_pos * 9);
            gl_draw_text_simple(cursor_text, cursor_x, 490, 16);
        }
        
        // Instructions - Split into two lines
        gl_set_color(0.4f, 0.3f, 0.2f);  // Darker brown for readability
        gl_draw_text_simple("Type your name (A-Z) | ENTER to save | BKSP to delete", 
                           580, 550, 11);
        gl_draw_text_simple("Max 32 characters", 
                           580, 568, 11);
    }
    
    // Render pause overlay if game is paused
    if (gui->game_paused && !gui->show_menu && !gui->visualizer.comet_buster.splash_screen_active) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Semi-transparent dark overlay (darker than menu to emphasize game pause)
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.5f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        // Pause dialog box
        int pause_x = 640;
        int pause_y = 300;
        int pause_width = 640;
        int pause_height = 480;
        
        // Dialog background - Dark with transparency
        gl_set_color(0.1f, 0.1f, 0.15f);
        gl_draw_rect_filled(pause_x, pause_y, pause_width, pause_height);
        
        // Glowing border - Gold/cyan gradient effect with multiple lines
        gl_set_color(1.0f, 0.8f, 0.0f);
        gl_draw_rect_filled(pause_x - 4, pause_y - 4, pause_width + 8, pause_height + 8);
        
        gl_set_color(0.0f, 1.0f, 1.0f);
        gl_draw_rect_filled(pause_x - 2, pause_y - 2, pause_width + 4, pause_height + 4);
        
        gl_set_color(0.1f, 0.1f, 0.15f);
        gl_draw_rect_filled(pause_x, pause_y, pause_width, pause_height);
        
        // PAUSED title - Large, glowing
        gl_set_color(1.0f, 1.0f, 0.0f);
        gl_draw_text_simple("PAUSED", 800, 380, 56);
        
        // Game continues in background info - Cyan
        gl_set_color(0.0f, 1.0f, 1.0f);
        gl_draw_text_simple("The game is paused", 810, 480, 18);
        
        // Stats while paused - White
        gl_set_color(1.0f, 1.0f, 1.0f);
        char pause_stats[256];
        sprintf(pause_stats, "Wave %d | Score %d | Lives %d",
                gui->visualizer.comet_buster.current_wave,
                gui->visualizer.comet_buster.score,
                gui->visualizer.comet_buster.ship_lives);
        gl_draw_text_simple(pause_stats, 750, 530, 16);
        
        // Resume instruction - Bright cyan, larger
        gl_set_color(0.2f, 1.0f, 0.8f);
        gl_draw_text_simple("Press P to Resume", 795, 600, 24);
        
        // Additional hints - Dimmer cyan
        gl_set_color(0.0f, 0.8f, 0.9f);
        gl_draw_text_simple("ESC for Menu", 835, 650, 16);
    }
    
    // Render cheat menu if open
    if (cheat_menu && cheat_menu->state == CHEAT_MENU_OPEN) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        // Dialog box background - Warm parchment
        int dialog_x = 460;
        int dialog_y = 200;
        int dialog_width = 1000;
        int dialog_height = 680;
        
        // Dark gray background
        gl_set_color(0.15f, 0.15f, 0.2f);
        gl_draw_rect_filled(dialog_x, dialog_y, dialog_width, dialog_height);
        
        // Border - Gold
        gl_set_color(0.8f, 0.6f, 0.2f);
        for (int i = 0; i < 3; i++) {
            gl_draw_rect_filled(dialog_x - i, dialog_y - i, dialog_width + 2*i, dialog_height + 2*i);
            if (i < 2) gl_set_color(0.8f, 0.6f, 0.2f);
        }
        
        // Title
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_text_simple("CHEAT MENU", 800, 230, 26);
        
        // Cheat options
        int option_y = 300;
        int line_height = 80;
        
        // Option 0: Wave
        gl_set_color(cheat_menu->selection == 0 ? 1.0f : 0.7f, 
                     cheat_menu->selection == 0 ? 1.0f : 0.7f, 
                     cheat_menu->selection == 0 ? 1.0f : 0.7f);
        char wave_text[128];
        sprintf(wave_text, "Wave: %d/30 (LEFT/RIGHT to adjust)", cheat_menu->wave);
        gl_draw_text_simple(wave_text, 550, option_y, 16);
        
        // Option 1: Lives
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 1 ? 1.0f : 0.7f,
                     cheat_menu->selection == 1 ? 1.0f : 0.7f,
                     cheat_menu->selection == 1 ? 1.0f : 0.7f);
        char lives_text[128];
        sprintf(lives_text, "Lives: %d/20 (LEFT/RIGHT to adjust)", cheat_menu->lives);
        gl_draw_text_simple(lives_text, 550, option_y, 16);
        
        // Option 2: Missiles
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 2 ? 1.0f : 0.7f,
                     cheat_menu->selection == 2 ? 1.0f : 0.7f,
                     cheat_menu->selection == 2 ? 1.0f : 0.7f);
        char missiles_text[128];
        sprintf(missiles_text, "Missiles: %d/99 (LEFT/RIGHT to adjust)", cheat_menu->missiles);
        gl_draw_text_simple(missiles_text, 550, option_y, 16);
        
        // Option 3: Bombs
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 3 ? 1.0f : 0.7f,
                     cheat_menu->selection == 3 ? 1.0f : 0.7f,
                     cheat_menu->selection == 3 ? 1.0f : 0.7f);
        char bombs_text[128];
        sprintf(bombs_text, "Bombs: %d/99 (LEFT/RIGHT to adjust)", cheat_menu->bombs);
        gl_draw_text_simple(bombs_text, 550, option_y, 16);

        // Option 4: Difficulty
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 4 ? 1.0f : 0.7f,
                     cheat_menu->selection == 4 ? 1.0f : 0.7f,
                     cheat_menu->selection == 4 ? 1.0f : 0.7f);
        char difficulty_text[128];
        switch (cheat_menu->cheat_difficulty) {
           case 0:
               sprintf(difficulty_text, "Difficulty: Easy (LEFT/RIGHT to adjust)");
               break;
           case 1:
               sprintf(difficulty_text, "Difficulty: Medium (LEFT/RIGHT to adjust)");
               break;
           case 2:
               sprintf(difficulty_text, "Difficulty: Hard (LEFT/RIGHT to adjust)");
               break;
            default:
               sprintf(difficulty_text, "Difficulty: Out of Range (LEFT/RIGHT to adjust)");
        }
        gl_draw_text_simple(difficulty_text, 550, option_y, 16);
        
        // Option 5: Apply
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 5 ? 0.2f : 0.1f,
                     cheat_menu->selection == 5 ? 1.0f : 0.5f,
                     cheat_menu->selection == 5 ? 0.2f : 0.1f);
        gl_draw_rect_filled(550, option_y - 10, 200, 35);
        gl_set_color(cheat_menu->selection == 5 ? 1.0f : 0.8f,
                     cheat_menu->selection == 5 ? 1.0f : 0.8f,
                     cheat_menu->selection == 5 ? 1.0f : 0.8f);
        gl_draw_text_simple("APPLY", 600, option_y+5, 14);
        
        // Option 6: Cancel
        option_y += 60;
        gl_set_color(cheat_menu->selection == 6 ? 1.0f : 0.5f,
                     cheat_menu->selection == 6 ? 0.2f : 0.1f,
                     cheat_menu->selection == 6 ? 0.2f : 0.1f);
        gl_draw_rect_filled(550, option_y - 10, 200, 35);
        gl_set_color(cheat_menu->selection == 6 ? 1.0f : 0.8f,
                     cheat_menu->selection == 6 ? 1.0f : 0.8f,
                     cheat_menu->selection == 6 ? 1.0f : 0.8f);
        gl_draw_text_simple("CANCEL", 600, option_y+5, 14);
        
    }
    
    SDL_GL_SwapWindow(gui->window);
}

static void cleanup(CometGUI *gui) {
    if (gui->joystick) SDL_JoystickClose(gui->joystick);
    if (gui->gl_context) SDL_GL_DeleteContext(gui->gl_context);
    if (gui->window) SDL_DestroyWindow(gui->window);
    SDL_Quit();
}

// ============================================================
// MAIN
// ============================================================

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("=== Comet Busters ===\n");
    
    CometGUI gui;
    memset(&gui, 0, sizeof(CometGUI));
    
    gui.window_width = 1024;
    gui.window_height = 768;
    gui.running = true;
    gui.music_volume = 100;
    gui.sfx_volume = 100;
    gui.finale_music_started = false;  // Initialize music state
    
    if (!init_sdl_and_opengl(&gui, gui.window_width, gui.window_height)) {
        return 1;
    }
    
    // Window is now maximized, get the actual size
    SDL_GetWindowSize(gui.window, &gui.window_width, &gui.window_height);
    printf("[INIT] Window maximized size: %dx%d\n", gui.window_width, gui.window_height);
    
    // Initialize visualizer with GAME space (1920x1080), not window size
    memset(&gui.visualizer, 0, sizeof(Visualizer));
    gui.visualizer.width = 1920;
    gui.visualizer.height = 1080;
    gui.visualizer.mouse_x = 960;
    gui.visualizer.mouse_y = 540;
    gui.visualizer.scroll_direction = 0;  // Initialize scroll wheel state
    
    printf("[INIT] Game initialized\n");
    
    // Load high scores
    high_scores_load(&gui.visualizer.comet_buster);
    printf("[INIT] High scores loaded\n");
    
    // Initialize audio system
    memset(&gui.audio, 0, sizeof(AudioManager));
    if (audio_init(&gui.audio)) {
        printf("[AUDIO] System initialized\n");
    } else {
        printf("[WARNING] Audio init failed\n");
    }
    
    // Load WAD file with sounds
    std::string wadPath;
#ifdef _WIN32
    wadPath = getExecutableDir() + "\\cometbuster.wad";
#else
    wadPath = "cometbuster.wad";
#endif
    
    if (audio_load_wad(&gui.audio, wadPath.c_str())) {
        printf("[AUDIO] WAD loaded: %s\n", wadPath.c_str());
    } else {
        printf("[WARNING] Could not load WAD: %s\n", wadPath.c_str());
    }
    
    // Copy audio to visualizer so game code can access it
    gui.visualizer.audio = gui.audio;
    
    // Apply loaded settings to audio system
    audio_set_music_volume(&gui.audio, gui.music_volume);
    audio_set_sfx_volume(&gui.audio, gui.sfx_volume);
    
    // Load background music tracks (for when gameplay starts)
#ifdef ExternalSound
    audio_play_music(&gui.audio, "music/track1.mp3", false);   // Load track 1
    audio_play_music(&gui.audio, "music/track2.mp3", false);   // Load track 2
    audio_play_music(&gui.audio, "music/track3.mp3", false);   // Load track 3
    audio_play_music(&gui.audio, "music/track4.mp3", false);   // Load track 4
    audio_play_music(&gui.audio, "music/track5.mp3", false);   // Load track 5
    audio_play_music(&gui.audio, "music/track6.mp3", false);   // Load track 6
    
    // Play intro music during splash screen
    audio_play_intro_music(&gui.audio, "music/intro.mp3");
#endif
    
    printf("[INIT] Ready to play - press WASD to move, Z to fire, ESC to open menu, P to pause\n");
    printf("[INIT] Starting with splash screen and intro music...\n");
    
    // Start with splash screen active
    gui.visualizer.comet_buster.splash_screen_active = true;
    comet_buster_reset_game_with_splash(&gui.visualizer.comet_buster, true, MEDIUM);

    gui.show_menu = false;
    gui.menu_state = 0;
    gui.gui_difficulty_level = 2;  // Medium
    
    // Initialize local high score entry UI
    HighScoreEntryUI hs_entry;
    memset(&hs_entry, 0, sizeof(HighScoreEntryUI));
    hs_entry.state = HIGH_SCORE_ENTRY_NONE;
    
    // Initialize cheat menu UI
    CheatMenuUI cheat_menu;
    memset(&cheat_menu, 0, sizeof(CheatMenuUI));
    cheat_menu.state = CHEAT_MENU_CLOSED;
    cheat_menu.selection = 0;
    cheat_menu.wave = 1;
    cheat_menu.lives = 3;
    cheat_menu.missiles = 0;
    cheat_menu.bombs = 0;
    
    // Main loop
    gui.last_frame_ticks = SDL_GetTicks();
    
    while (gui.running) {
        uint32_t current_ticks = SDL_GetTicks();
        gui.delta_time = (current_ticks - gui.last_frame_ticks) / 1000.0;
        gui.last_frame_ticks = current_ticks;
        
        if (gui.delta_time > 0.033) gui.delta_time = 0.033;
        
        gui.total_time += gui.delta_time;
        gui.frame_count++;
        
        handle_events(&gui, &hs_entry, &cheat_menu);
        update_game(&gui, &hs_entry);
        
        // Reset scroll wheel input after processing (for weapon changing)
        gui.visualizer.scroll_direction = 0;
        
        // Reset mouse_just_moved flag for next frame
        //gui.visualizer.mouse_just_moved = false;
        
        render_frame(&gui, &hs_entry, &cheat_menu);
        
        uint32_t elapsed = SDL_GetTicks() - current_ticks;
        if (elapsed < 16) SDL_Delay(16 - elapsed);
    }
    
    cleanup(&gui);
    return 0;
}
