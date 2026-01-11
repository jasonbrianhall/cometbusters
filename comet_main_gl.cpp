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
#include "comet_help.h"

// Forward declare functions from visualization.h and other headers
extern void update_comet_buster(Visualizer *vis_ptr, double dt);
extern void draw_comet_buster_gl(Visualizer *visualizer, void *cr);
extern void high_scores_load(CometBusterGame *game);
extern void audio_set_music_volume(AudioManager *audio, int volume);
extern void audio_set_sfx_volume(AudioManager *audio, int volume);
extern void comet_buster_spawn_wave(CometBusterGame *game, int screen_width, int screen_height);
extern void audio_play_random_music(AudioManager *audio);

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
    int difficulty_level; // 1-3
    
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

static void handle_keyboard_input(SDL_Event *event, CometGUI *gui) {
    if (!gui) return;
    bool pressed = (event->type == SDL_KEYDOWN);
    
    // Disable mouse aim when keyboard is used
    if (pressed) {
        gui->visualizer.mouse_just_moved = false;
    }
    
    switch (event->key.keysym.sym) {
        // Movement keys - A/D/W/S and Arrow keys
        case SDLK_a:
        case SDLK_LEFT:
            gui->visualizer.comet_buster.keyboard.key_a_pressed = pressed;
            break;
        case SDLK_d:
        case SDLK_RIGHT:
            gui->visualizer.comet_buster.keyboard.key_d_pressed = pressed;
            break;
        case SDLK_w:
        case SDLK_UP:
            gui->visualizer.comet_buster.keyboard.key_w_pressed = pressed;
            break;
        case SDLK_s:
        case SDLK_DOWN:
            gui->visualizer.comet_buster.keyboard.key_s_pressed = pressed;
            break;
        
        // Fire keys
        case SDLK_z:
            gui->visualizer.comet_buster.keyboard.key_z_pressed = pressed;
            break;
        case SDLK_x:
            gui->visualizer.comet_buster.keyboard.key_x_pressed = pressed;
            break;
        case SDLK_SPACE:
            gui->visualizer.comet_buster.keyboard.key_space_pressed = pressed;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            gui->visualizer.comet_buster.keyboard.key_ctrl_pressed = pressed;
            break;
        
        // Special keys
        case SDLK_q:
            gui->visualizer.comet_buster.keyboard.key_q_pressed = pressed;
            break;
        case SDLK_v:
            // V opens volume dialog (would need implementation)
            if (pressed) {
                printf("[INPUT] V key pressed - volume dialog not yet implemented in OpenGL version\n");
            }
            break;
        case SDLK_F11:
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
            // P key to pause (ESC is now for menu)
            if (!gui->visualizer.comet_buster.splash_screen_active) {
                gui->game_paused = !gui->game_paused;
                printf(gui->game_paused ? "[*] Game Paused\n" : "[*] Game Resumed\n");
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

static void handle_joystick_input(SDL_Event *event, CometGUI *gui) {
    if (!gui || !gui->joystick) return;
    const int DEADZONE = 8000;
    
    switch (event->type) {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP: {
            bool pressed = (event->type == SDL_JOYBUTTONDOWN);
            switch (event->jbutton.button) {
                case 0: gui->visualizer.comet_buster.keyboard.key_z_pressed = pressed; break;
                case 1: gui->visualizer.comet_buster.keyboard.key_x_pressed = pressed; break;
                case 2: gui->visualizer.comet_buster.keyboard.key_space_pressed = pressed; break;
            }
            break;
        }
        case SDL_JOYAXISMOTION: {
            int16_t value = event->jaxis.value;
            if (event->jaxis.axis == 0) {
                if (value < -DEADZONE) {
                    gui->visualizer.comet_buster.keyboard.key_a_pressed = true;
                    gui->visualizer.comet_buster.keyboard.key_d_pressed = false;
                } else if (value > DEADZONE) {
                    gui->visualizer.comet_buster.keyboard.key_d_pressed = true;
                    gui->visualizer.comet_buster.keyboard.key_a_pressed = false;
                } else {
                    gui->visualizer.comet_buster.keyboard.key_a_pressed = false;
                    gui->visualizer.comet_buster.keyboard.key_d_pressed = false;
                }
            }
            else if (event->jaxis.axis == 1) {
                if (value < -DEADZONE) {
                    gui->visualizer.comet_buster.keyboard.key_w_pressed = true;
                    gui->visualizer.comet_buster.keyboard.key_s_pressed = false;
                } else if (value > DEADZONE) {
                    gui->visualizer.comet_buster.keyboard.key_s_pressed = true;
                    gui->visualizer.comet_buster.keyboard.key_w_pressed = false;
                } else {
                    gui->visualizer.comet_buster.keyboard.key_w_pressed = false;
                    gui->visualizer.comet_buster.keyboard.key_s_pressed = false;
                }
            }
            break;
        }
        case SDL_JOYHATMOTION: {
            uint8_t hat = event->jhat.value;
            gui->visualizer.comet_buster.keyboard.key_w_pressed = (hat & SDL_HAT_UP) != 0;
            gui->visualizer.comet_buster.keyboard.key_s_pressed = (hat & SDL_HAT_DOWN) != 0;
            gui->visualizer.comet_buster.keyboard.key_a_pressed = (hat & SDL_HAT_LEFT) != 0;
            gui->visualizer.comet_buster.keyboard.key_d_pressed = (hat & SDL_HAT_RIGHT) != 0;
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

static void handle_events(CometGUI *gui) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                gui->running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // Escape toggles menu
                    if (gui->show_menu && gui->menu_state != 0) {
                        gui->menu_state = 0;  // Go back to main menu
                        gui->menu_selection = 1;  // Reset to New Game
                    } else {
                        gui->show_menu = !gui->show_menu;
                        gui->menu_state = 0;
                        gui->menu_selection = 0;  // Reset to Continue option
                    }
                } else if (gui->show_menu) {
                    // Menu is open - handle navigation
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection - 1 + 5) % 5;
                        } else if (gui->menu_state == 1) {  // Difficulty selection
                            gui->difficulty_level = (gui->difficulty_level - 1);
                            if (gui->difficulty_level < 1) gui->difficulty_level = 3;
                        } else if (gui->menu_state == 3) {  // Audio menu
                            gui->menu_selection = (gui->menu_selection - 1 + 2) % 2;
                        }
                    } else if (event.key.keysym.sym == SDLK_DOWN) {
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection + 1) % 5;
                        } else if (gui->menu_state == 1) {  // Difficulty selection
                            gui->difficulty_level = (gui->difficulty_level % 3) + 1;
                        } else if (gui->menu_state == 3) {  // Audio menu
                            gui->menu_selection = (gui->menu_selection + 1) % 2;
                        }
                    } else if (event.key.keysym.sym == SDLK_LEFT) {
                        if (gui->menu_state == 3) {  // Audio menu
                            if (gui->menu_selection == 0) {  // Music
                                gui->music_volume = (gui->music_volume >= 10) ? gui->music_volume - 10 : 0;
                                audio_set_music_volume(&gui->audio, gui->music_volume);
                            } else {  // SFX
                                gui->sfx_volume = (gui->sfx_volume >= 10) ? gui->sfx_volume - 10 : 0;
                                audio_set_sfx_volume(&gui->audio, gui->sfx_volume);
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_RIGHT) {
                        if (gui->menu_state == 3) {  // Audio menuf
                            if (gui->menu_selection == 0) {  // Music
                                gui->music_volume = (gui->music_volume <= 90) ? gui->music_volume + 10 : 100;
                                audio_set_music_volume(&gui->audio, gui->music_volume);
                            } else {  // SFX
                                gui->sfx_volume = (gui->sfx_volume <= 90) ? gui->sfx_volume + 10 : 100;
                                audio_set_sfx_volume(&gui->audio, gui->sfx_volume);
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        // Select menu option
                        if (gui->menu_state == 0) {
                            // Main menu
                            switch (gui->menu_selection) {
                                case 0:  // Continue
                                    gui->show_menu = false;
                                    break;
                                case 1:  // New Game - go to difficulty selection
                                    gui->menu_state = 1;
                                    gui->difficulty_level = 1;
                                    break;
                                case 2:  // High Scores - show high scores
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
                            // Difficulty selected - start new game
                            comet_buster_reset_game(&gui->visualizer.comet_buster);
                            comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
                            
                            // Start background music
                            audio_play_random_music(&gui->audio);
                            
                            gui->show_menu = false;
                            gui->menu_state = 0;
                            printf("[MENU] New game started with difficulty %d\n", gui->difficulty_level);
                        } else if (gui->menu_state == 2) {
                            // Go back from high scores
                            gui->menu_state = 0;
                            gui->menu_selection = 2;
                        } else if (gui->menu_state == 3) {
                            // Go back from audio
                            gui->menu_state = 0;
                            gui->menu_selection = 3;
                        }
                    } else {
                        // Regular keyboard input when menu is open but we're navigating
                        handle_keyboard_input(&event, gui);
                    }
                } else {
                    // Game is running - handle all input
                    handle_keyboard_input(&event, gui);
                    handle_keyboard_input_special(&event, gui);
                }
                break;
            case SDL_KEYUP:
                handle_keyboard_input(&event, gui);
                break;
            case SDL_MOUSEMOTION: {
                // Scale mouse from window space (1024x768) to game space (1920x1080)
                float scale_x = 1920.0f / gui->window_width;
                float scale_y = 1080.0f / gui->window_height;
                gui->visualizer.mouse_x = (int)(event.motion.x * scale_x);
                gui->visualizer.mouse_y = (int)(event.motion.y * scale_y);
                gui->visualizer.mouse_just_moved = true;
                gui->visualizer.mouse_movement_timer = 0.1f;  // 100ms
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    gui->visualizer.mouse_left_pressed = true;
                    
                    // Handle menu clicks if menu is open
                    if (gui->show_menu) {
                        float scale_x = 1920.0f / gui->window_width;
                        float scale_y = 1080.0f / gui->window_height;
                        int mouse_x = (int)(event.button.x * scale_x);
                        int mouse_y = (int)(event.button.y * scale_y);
                        
                        // Main menu buttons
                        if (gui->menu_state == 0) {
                            const int menu_y_start = 250;
                            const int menu_spacing = 70;
                            const int menu_width = 400;
                            const int menu_height = 50;
                            const int menu_x = (1920 - menu_width) / 2;
                            
                            for (int i = 0; i < 5; i++) {
                                int option_y = menu_y_start + (i * menu_spacing);
                                if (mouse_x >= menu_x && mouse_x <= menu_x + menu_width &&
                                    mouse_y >= option_y && mouse_y <= option_y + menu_height) {
                                    gui->menu_selection = i;
                                    // Trigger selection
                                    switch (i) {
                                        case 0: gui->show_menu = false; break;  // Continue
                                        case 1: gui->menu_state = 1; gui->difficulty_level = 1; break;  // New Game
                                        case 2: gui->menu_state = 2; break;  // High Scores
                                        case 3: gui->menu_state = 3; gui->menu_selection = 0; break;  // Audio
                                        case 4: gui->running = false; break;  // Quit
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
                                    gui->difficulty_level = i + 1;
                                    // Start game with selected difficulty
                                    comet_buster_reset_game(&gui->visualizer.comet_buster);
                                    comet_buster_spawn_wave(&gui->visualizer.comet_buster, 1920, 1080);
                                    audio_play_random_music(&gui->audio);
                                    gui->show_menu = false;
                                    gui->menu_state = 0;
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
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    gui->visualizer.mouse_right_pressed = true;
                    
                    // Right-click to restart game if game is over
                    if (gui->visualizer.comet_buster.game_over) {
                        printf("[GAME] Restarting game via right-click...\n");
                        comet_buster_reset_game(&gui->visualizer.comet_buster);
                    }
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    // Middle button press
                    gui->visualizer.mouse_middle_pressed = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    gui->visualizer.mouse_left_pressed = false;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    gui->visualizer.mouse_right_pressed = false;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    // Middle button release
                    gui->visualizer.mouse_middle_pressed = false;
                }
                break;
            case SDL_MOUSEWHEEL:
                // Scroll wheel works for menu navigation or weapon changing
                if (gui->show_menu) {
                    // Menu is open - scroll navigates menu
                    if (event.wheel.y > 0) {
                        // Scroll up = move up in menu
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection - 1 + 5) % 5;
                        } else if (gui->menu_state == 1) {  // Difficulty selection
                            gui->difficulty_level = (gui->difficulty_level - 1);
                            if (gui->difficulty_level < 1) gui->difficulty_level = 3;
                        } else if (gui->menu_state == 3) {  // Audio menu
                            gui->menu_selection = (gui->menu_selection - 1 + 2) % 2;
                        }
                    } else if (event.wheel.y < 0) {
                        // Scroll down = move down in menu
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection + 1) % 5;
                        } else if (gui->menu_state == 1) {  // Difficulty selection
                            gui->difficulty_level = (gui->difficulty_level % 3) + 1;
                        } else if (gui->menu_state == 3) {  // Audio menu
                            gui->menu_selection = (gui->menu_selection + 1) % 2;
                        }
                    }
                } else {
                    // Game is running - scroll changes weapons
                    if (event.wheel.y > 0) {
                        gui->visualizer.scroll_direction = 1;   // Previous weapon
                    } else if (event.wheel.y < 0) {
                        gui->visualizer.scroll_direction = -1;  // Next weapon
                    }
                }
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYHATMOTION:
                handle_joystick_input(&event, gui);
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    gui->window_width = event.window.data1;
                    gui->window_height = event.window.data2;
                    printf("[WINDOW] Resized to %dx%d\n", gui->window_width, gui->window_height);
                } else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                    SDL_GetWindowSize(gui->window, &gui->window_width, &gui->window_height);
                    printf("[WINDOW] Maximized to %dx%d\n", gui->window_width, gui->window_height);
                } else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    SDL_GetWindowSize(gui->window, &gui->window_width, &gui->window_height);
                    printf("[WINDOW] Restored to %dx%d\n", gui->window_width, gui->window_height);
                }
                break;
        }
    }
}

static void update_game(CometGUI *gui) {
    // Don't update if menu is open or game is paused
    if (gui->show_menu || gui->game_paused) return;
    
    // Call the master update function from visualization.h
    // This handles ALL game updates including collisions, audio, wave progression, etc.
    update_comet_buster(&gui->visualizer, gui->delta_time);
}

static void render_frame(CometGUI *gui) {
    glClearColor(0.05f, 0.075f, 0.15f, 1.0f);  // Dark blue for all screens
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Calculate scaling to fit window while maintaining 1920x1080 aspect ratio
    const float GAME_WIDTH = 1920.0f;
    const float GAME_HEIGHT = 1080.0f;
    
    float window_aspect = (float)gui->window_width / gui->window_height;
    float game_aspect = GAME_WIDTH / GAME_HEIGHT;
    
    int viewport_x = 0;
    int viewport_y = 0;
    int viewport_width = gui->window_width;
    int viewport_height = gui->window_height;
    
    // Scale to fit, aligned to top-left
    if (window_aspect > game_aspect) {
        // Window is wider than game aspect - fit to height, align left
        viewport_height = gui->window_height;
        viewport_width = (int)(GAME_WIDTH * gui->window_height / GAME_HEIGHT);
        viewport_x = 0;
        viewport_y = 0;
    } else {
        // Window is narrower - fit to width
        viewport_width = gui->window_width;
        viewport_height = (int)(GAME_HEIGHT * gui->window_width / GAME_WIDTH);
        viewport_x = 0;
        viewport_y = 0;
    }
    
    // Set viewport to scaled region
    glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
    
    // Update visualizer dimensions for display
    gui->visualizer.width = 1920;
    gui->visualizer.height = 1080;
    
    // Call the master render function
    draw_comet_buster_gl(&gui->visualizer, NULL);
    
    // Render menu overlay if open
    if (gui->show_menu) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Darken background
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        if (gui->menu_state == 0) {
            // Main menu
            gl_set_color(0.0f, 1.0f, 1.0f);
            gl_draw_text_simple("COMET BUSTERS", 850, 100, 20);
            
            const char *options[] = {
                "CONTINUE",
                "NEW GAME",
                "HIGH SCORES",
                "AUDIO",
                "QUIT"
            };
            
            int menu_y_start = 250;
            int menu_spacing = 70;
            int menu_width = 400;
            int menu_height = 50;
            
            for (int i = 0; i < 5; i++) {
                int option_y = menu_y_start + (i * menu_spacing);
                int option_x = (1920 - menu_width) / 2;
                
                if (i == gui->menu_selection) {
                    gl_set_color(1.0f, 1.0f, 0.0f);
                    gl_draw_rect_filled(option_x - 3, option_y - 3, menu_width + 6, menu_height + 6);
                    gl_set_color(0.0f, 0.5f, 1.0f);
                    gl_draw_rect_filled(option_x, option_y, menu_width, menu_height);
                    gl_set_color(1.0f, 1.0f, 0.0f);
                } else {
                    gl_set_color(0.2f, 0.2f, 0.4f);
                    gl_draw_rect_filled(option_x, option_y, menu_width, menu_height);
                    gl_set_color(0.7f, 0.7f, 1.0f);
                }
                
                gl_draw_text_simple(options[i], option_x + 80, option_y + 12, 14);
            }
            
            gl_set_color(0.8f, 0.8f, 0.8f);
            gl_draw_text_simple("UP/DOWN to select | ENTER to choose | ESC to close", 550, 950, 10);
            
        } else if (gui->menu_state == 1) {
            // Difficulty selection
            gl_set_color(0.0f, 1.0f, 1.0f);
            gl_draw_text_simple("SELECT DIFFICULTY", 800, 150, 18);
            
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
                
                if ((i + 1) == gui->difficulty_level) {
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
    
    // Initialize game - NO SPLASH, go straight to gameplay
    comet_buster_reset_game(&gui.visualizer.comet_buster);
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
    
    // Set volumes
    audio_set_music_volume(&gui.audio, gui.music_volume);
    audio_set_sfx_volume(&gui.audio, gui.sfx_volume);
    
    printf("[INIT] Ready to play - press W/A/D/S to move, Z to fire, ESC to quit, P to pause\n");
    printf("[INIT] Skipping splash screen...\n");
    
    // SKIP SPLASH SCREEN - disable it immediately
    gui.visualizer.comet_buster.splash_screen_active = false;
    gui.visualizer.comet_buster.splash_timer = 0.0;
    
    // Start game directly on medium difficulty (skip menu)
    comet_buster_reset_game(&gui.visualizer.comet_buster);
    comet_buster_spawn_wave(&gui.visualizer.comet_buster, 1920, 1080);
    audio_play_random_music(&gui.audio);
    gui.show_menu = false;
    gui.menu_state = 0;
    gui.difficulty_level = 2;  // Medium
    printf("[INIT] Game starting directly on MEDIUM difficulty\n");
    
    // Main loop
    gui.last_frame_ticks = SDL_GetTicks();
    
    while (gui.running) {
        uint32_t current_ticks = SDL_GetTicks();
        gui.delta_time = (current_ticks - gui.last_frame_ticks) / 1000.0;
        gui.last_frame_ticks = current_ticks;
        
        if (gui.delta_time > 0.033) gui.delta_time = 0.033;
        
        gui.total_time += gui.delta_time;
        gui.frame_count++;
        
        handle_events(&gui);
        update_game(&gui);
        
        // Reset scroll wheel input after processing (for weapon changing)
        gui.visualizer.scroll_direction = 0;
        
        render_frame(&gui);
        
        uint32_t elapsed = SDL_GetTicks() - current_ticks;
        if (elapsed < 16) SDL_Delay(16 - elapsed);
    }
    
    cleanup(&gui);
    return 0;
}
