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
extern void audio_stop_music(AudioManager *audio);
extern void audio_play_music(AudioManager *audio, const char *internal_path, bool loop);
extern void audio_play_intro_music(AudioManager *audio, const char *internal_path);
extern bool audio_is_music_playing(AudioManager *audio);

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
            // P key to pause - check three conditions
            // Only allow pause if game is not over AND splash screen is not active AND finale is not active
            if (!gui->visualizer.comet_buster.game_over && 
                !gui->visualizer.comet_buster.splash_screen_active &&
                !gui->visualizer.comet_buster.finale_splash_active) {
                gui->game_paused = !gui->game_paused;
                
                // Stop music immediately when pausing
                if (gui->game_paused) {
                    audio_stop_music(&gui->audio);
                    printf("[*] Game Paused\n");
                } else {
                    printf("[*] Game Resumed\n");
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

static void handle_events(CometGUI *gui) {
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
                
                // Handle menu navigation
                if (gui->show_menu) {
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection - 1 + 5) % 5;
                        } else if (gui->menu_state == 1) {
                            gui->difficulty_level = (gui->difficulty_level - 1);
                            if (gui->difficulty_level < 1) gui->difficulty_level = 3;
                        } else if (gui->menu_state == 3) {
                            gui->menu_selection = (gui->menu_selection - 1 + 2) % 2;
                        }
                    } else if (event.key.keysym.sym == SDLK_DOWN) {
                        if (gui->menu_state == 0) {
                            gui->menu_selection = (gui->menu_selection + 1) % 5;
                        } else if (gui->menu_state == 1) {
                            gui->difficulty_level = (gui->difficulty_level + 1);
                            if (gui->difficulty_level > 3) gui->difficulty_level = 1;
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
                                if (gui->music_volume > 128) gui->music_volume = 128;
                                audio_set_music_volume(&gui->audio, gui->music_volume);
                            } else {
                                gui->sfx_volume = (gui->sfx_volume + 5);
                                if (gui->sfx_volume > 128) gui->sfx_volume = 128;
                                audio_set_sfx_volume(&gui->audio, gui->sfx_volume);
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_RETURN) {
                        if (gui->menu_state == 0) {
                            switch (gui->menu_selection) {
                                case 0:  // Continue
                                    gui->show_menu = false;
                                    break;
                                case 1:  // New Game - go to difficulty selection
                                    gui->menu_state = 1;
                                    gui->difficulty_level = 1;
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
                            comet_buster_reset_game(&gui->visualizer.comet_buster);
                            gui->visualizer.comet_buster.splash_screen_active = true;
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
                    handle_keyboard_input(&event, gui);
                    handle_keyboard_input_special(&event, gui);
                }
                break;
            }
            
            case SDL_KEYUP:
                if (!gui->show_menu) {
                    handle_keyboard_input(&event, gui);
                }
                break;
            
            case SDL_MOUSEBUTTONDOWN: {
                // Check for splash screen exit on mouse click
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
                    // Calculate viewport same as render_frame does for consistent scaling
                    const float GAME_WIDTH = 1920.0f;
                    const float GAME_HEIGHT = 1080.0f;
                    float window_aspect = (float)gui->window_width / gui->window_height;
                    float game_aspect = GAME_WIDTH / GAME_HEIGHT;
                    
                    int viewport_x = 0;
                    int viewport_y = 0;
                    int viewport_width = gui->window_width;
                    int viewport_height = gui->window_height;
                    
                    if (window_aspect > game_aspect) {
                        viewport_height = gui->window_height;
                        viewport_width = (int)(GAME_WIDTH * gui->window_height / GAME_HEIGHT);
                        viewport_x = 0;
                        viewport_y = 0;
                    } else {
                        viewport_width = gui->window_width;
                        viewport_height = (int)(GAME_HEIGHT * gui->window_width / GAME_WIDTH);
                        viewport_x = 0;
                        viewport_y = 0;
                    }
                    
                    // Convert from window coordinates to game space
                    float mouse_in_viewport_x = event.button.x - viewport_x;
                    float mouse_in_viewport_y = event.button.y - viewport_y;
                    
                    float scale_x = GAME_WIDTH / viewport_width;
                    float scale_y = GAME_HEIGHT / viewport_height;
                    
                    int mouse_x = (int)(mouse_in_viewport_x * scale_x);
                    int mouse_y = (int)(mouse_in_viewport_y * scale_y);
                    
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
                                            gui->difficulty_level = 1;
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
                                    gui->difficulty_level = i + 1;
                                    // Start game with selected difficulty
                                    comet_buster_reset_game(&gui->visualizer.comet_buster);
                                    gui->visualizer.comet_buster.splash_screen_active = true;
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
                            comet_buster_reset_game(&gui->visualizer.comet_buster);
                            gui->visualizer.comet_buster.splash_screen_active = true;
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
                // Convert window pixel coordinates to game logical coordinates
                // Window fills entire screen, so scaling is direct
                gui->visualizer.mouse_x = (int)(event.motion.x * 1920.0f / gui->window_width);
                gui->visualizer.mouse_y = (int)(event.motion.y * 1080.0f / gui->window_height);
                gui->visualizer.mouse_just_moved = true;
                break;
            }
            
            case SDL_MOUSEWHEEL:
                gui->visualizer.scroll_direction = event.wheel.y;
                break;
            
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                init_joystick(gui);
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
    
    // Check if current music track has finished and queue the next one
#ifdef ExternalSound
    if (!gui->game_paused && !audio_is_music_playing(&gui->audio)) {
        fprintf(stdout, "[AUDIO] Current track finished, queuing next track...\n");
        audio_play_random_music(&gui->audio);
    }
#endif
    
    // Handle finale splash if active (Wave 30 victory)
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
        
        // Check if user wants to skip finale (right-click to continue)
        if (gui->visualizer.mouse_right_pressed) {
            fprintf(stdout, "[FINALE] Player skipping finale\n");
            
            // Stop the finale music
            audio_stop_music(&gui->audio);
            
            // Clean up finale splash
            gui->visualizer.comet_buster.finale_splash_active = false;
            gui->finale_music_started = false;
        }
    }
    
    // Stop music if game ends
    if (gui->visualizer.comet_buster.game_over) {
        audio_stop_music(&gui->audio);
    }
}

static void render_frame(CometGUI *gui) {
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
                
                gl_draw_text_simple(menu_items[i], menu_x + 110, menu_y + 15, 14);
            }
            
            gl_set_color(0.8f, 0.8f, 0.8f);
            gl_draw_text_simple("UP/DOWN to select | ENTER to start | ESC to close", 550, 950, 10);
            
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
    
    // Initialize game
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
    
    printf("[INIT] Ready to play - press W/A/D/S to move, Z to fire, ESC to open menu, P to pause\n");
    printf("[INIT] Starting with splash screen and intro music...\n");
    
    // Start with splash screen active
    gui.visualizer.comet_buster.splash_screen_active = true;
    comet_buster_reset_game(&gui.visualizer.comet_buster);
    gui.show_menu = false;
    gui.menu_state = 0;
    gui.difficulty_level = 2;  // Medium
    
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
