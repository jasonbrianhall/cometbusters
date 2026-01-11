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

// Forward declare GL functions
extern void gl_setup_2d_projection(int width, int height);
extern void draw_comet_buster_gl(Visualizer *visualizer, void *cr);

// Forward declare audio functions
extern void high_scores_load(CometBusterGame *game);
extern void audio_set_music_volume(AudioManager *audio, int volume);
extern void audio_set_sfx_volume(AudioManager *audio, int volume);

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
    
    switch (event->key.keysym.sym) {
        case SDLK_w:
            gui->visualizer.comet_buster.keyboard.key_w_pressed = pressed;
            break;
        case SDLK_a:
            gui->visualizer.comet_buster.keyboard.key_a_pressed = pressed;
            break;
        case SDLK_d:
            gui->visualizer.comet_buster.keyboard.key_d_pressed = pressed;
            break;
        case SDLK_s:
            gui->visualizer.comet_buster.keyboard.key_s_pressed = pressed;
            break;
        case SDLK_z:
            gui->visualizer.comet_buster.keyboard.key_z_pressed = pressed;
            break;
        case SDLK_x:
            gui->visualizer.comet_buster.keyboard.key_x_pressed = pressed;
            break;
        case SDLK_SPACE:
            gui->visualizer.comet_buster.keyboard.key_space_pressed = pressed;
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            gui->visualizer.comet_buster.keyboard.key_space_pressed = pressed;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            gui->visualizer.comet_buster.keyboard.key_ctrl_pressed = pressed;
            break;
        case SDLK_q:
            gui->visualizer.comet_buster.keyboard.key_q_pressed = pressed;
            break;
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
    
    gui->window = SDL_CreateWindow(
        "CometBuster - SDL2+OpenGL",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    
    if (!gui->window) {
        fprintf(stderr, "[ERROR] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
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
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
                    gui->running = false;
                } else if (event.key.keysym.sym == SDLK_p) {
                    gui->game_paused = !gui->game_paused;
                } else {
                    handle_keyboard_input(&event, gui);
                }
                break;
            case SDL_KEYUP:
                handle_keyboard_input(&event, gui);
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
                }
                break;
        }
    }
}

static void update_game(CometGUI *gui) {
    if (gui->game_paused) return;
    
    CometBusterGame *game = &gui->visualizer.comet_buster;
    
    // Don't update if game over - just let it display
    if (game->game_over) return;
    
    // Update all game systems
    comet_buster_update_ship(game, gui->delta_time, 0, 0, gui->window_width, gui->window_height, false);
    comet_buster_update_comets(game, gui->delta_time, gui->window_width, gui->window_height);
    comet_buster_update_bullets(game, gui->delta_time, gui->window_width, gui->window_height, &gui->visualizer);
    comet_buster_update_particles(game, gui->delta_time);
    comet_buster_update_floating_text(game, gui->delta_time);
    comet_buster_update_canisters(game, gui->delta_time);
    comet_buster_update_missile_pickups(game, gui->delta_time);
    comet_buster_update_missiles(game, gui->delta_time, gui->window_width, gui->window_height);
    comet_buster_update_bomb_pickups(game, gui->delta_time);
    comet_buster_update_bombs(game, gui->delta_time, gui->window_width, gui->window_height, &gui->visualizer);
    comet_buster_update_enemy_bullets(game, gui->delta_time, gui->window_width, gui->window_height, &gui->visualizer);
    comet_buster_update_burner_effects(game, gui->delta_time);
    
    // Update other systems
    if (game->boss_active) {
        comet_buster_update_boss(game, gui->delta_time, gui->window_width, gui->window_height);
    }
    if (game->spawn_queen.active) {
        comet_buster_update_spawn_queen(game, gui->delta_time, gui->window_width, gui->window_height);
    }
}

static void render_frame(CometGUI *gui) {
    glViewport(0, 0, gui->window_width, gui->window_height);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Update visualizer dimensions
    gui->visualizer.width = gui->window_width;
    gui->visualizer.height = gui->window_height;
    
    // Call the master render function - this handles everything
    draw_comet_buster_gl(&gui->visualizer, NULL);
    
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
    printf("=== CometBuster SDL2+OpenGL ===\n");
    
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
    
    // Initialize visualizer
    memset(&gui.visualizer, 0, sizeof(Visualizer));
    gui.visualizer.width = gui.window_width;
    gui.visualizer.height = gui.window_height;
    gui.visualizer.mouse_x = gui.window_width / 2;
    gui.visualizer.mouse_y = gui.window_height / 2;
    
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
    
    // Main loop
    gui.last_frame_ticks = SDL_GetTicks();
    
    while (gui.running) {
        uint32_t current_ticks = SDL_GetTicks();
        gui.delta_time = (current_ticks - gui.last_frame_ticks) / 1000.0;
        gui.last_frame_ticks = current_ticks;
        
        if (gui.delta_time > 0.033) gui.delta_time = 0.033;
        
        gui.total_time += gui.delta_time;
        gui.frame_count++;
        
        if ((int)gui.total_time > 0 && gui.frame_count % 60 == 0) {
            printf("[FPS] %.1f\n", gui.frame_count / gui.total_time);
        }
        
        handle_events(&gui);
        update_game(&gui);
        render_frame(&gui);
        
        uint32_t elapsed = SDL_GetTicks() - current_ticks;
        if (elapsed < 16) SDL_Delay(16 - elapsed);
    }
    
    cleanup(&gui);
    return 0;
}
