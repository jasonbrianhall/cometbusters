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

void play_intro(CometGUI *gui, int language) {
    audio_play_intro_music(&gui->audio, wlanguage_intro_file[language]);
}

void play_finale(CometGUI *gui, int language) {
    audio_play_intro_music(&gui->audio, wlanguage_finale_file[language]);
}


static bool init_sdl_and_opengl(CometGUI *gui, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        SDL_Log("[Comet Busters] [ERROR] SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    
    // Enable joystick events
    SDL_JoystickEventState(SDL_ENABLE);
    
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
        SDL_Log("[Comet Busters] [ERROR] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // Ensure window is visible on all platforms
    SDL_ShowWindow(gui->window);
    SDL_RaiseWindow(gui->window);
    
    // Get the actual window size (might be different if maximized)
    SDL_GetWindowSize(gui->window, &gui->window_width, &gui->window_height);
    SDL_Log("[Comet Busters] [SDL] Window created: %dx%d\n", gui->window_width, gui->window_height);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
#ifdef ANDROID
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    gui->gl_context = SDL_GL_CreateContext(gui->window);
    if (!gui->gl_context) {
        SDL_Log("[Comet Busters] [ERROR] GL context failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        return false;
    }
    
    SDL_GL_SetSwapInterval(1);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        SDL_Log("[Comet Busters] [ERROR] GLEW init failed\n");
        SDL_GL_DeleteContext(gui->gl_context);
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        return false;
    }
    
    SDL_Log("[Comet Busters] [INIT] SDL2, OpenGL %s, GLEW OK\n", glGetString(GL_VERSION));
    
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

static void update_game(CometGUI *gui, HighScoreEntryUI *hs_entry) {
    // Don't update if menu is open or game is paused
    if (gui->show_menu || gui->game_paused) return;
    
    // Handle finale splash if active (Wave 30 victory)
    // UPDATE THIS FIRST before skipping normal game update
    if (gui->visualizer.comet_buster.finale_splash_active) {
        // Start finale music on first frame of finale splash
        if (!gui->finale_music_started) {
            SDL_Log("[Comet Busters] [FINALE] Starting finale music...\n");
            audio_stop_music(&gui->audio);
#ifdef ExternalSound
            //audio_play_music(&gui->audio, "music/finale.mp3", false);  // Don't loop
            play_finale(gui, gui->visualizer.comet_buster.current_language);
#endif
            gui->finale_music_started = true;
        }
        
        // Update the finale splash (THIS IS CRITICAL - animates the finale)
        comet_buster_update_finale_splash(&gui->visualizer.comet_buster, 1.0 / 60.0);
        
        // Check if user wants to continue to next wave (can right-click anytime to skip)
        if (gui->visualizer.mouse_right_pressed) {
            SDL_Log("[Comet Busters] [FINALE] Player skipping to Wave 31\n");
            
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
        SDL_Log("[Comet Busters] [AUDIO] Current track finished, queuing next track...\n");
        audio_play_random_music(&gui->audio);
    }
#endif
    
    // Stop music if game ends and trigger high score entry
    if (gui->visualizer.comet_buster.game_over || gui->visualizer.comet_buster.ship_lives <=0) {
        SDL_Log("[Comet Busters] \n[HS_FLOW] >>> GAME OVER DETECTED\n");
        audio_stop_music(&gui->audio);
        
        // Trigger high score entry if not already showing the dialog
        if (!hs_entry) {
            SDL_Log("[Comet Busters] [HS_FLOW] ERROR: hs_entry is NULL!\n");
            return;
        }
        
        SDL_Log("[Comet Busters] [HS_FLOW] hs_entry->state = %d (0=NONE, 1=ACTIVE, 2=SAVED)\n", hs_entry->state);
        
        if (hs_entry->state != HIGH_SCORE_ENTRY_ACTIVE) {
            int score = gui->visualizer.comet_buster.score;
            int count = gui->visualizer.comet_buster.high_score_count;
            
            SDL_Log("[Comet Busters] [HS_FLOW] Checking high score eligibility...\n");
            SDL_Log("[Comet Busters] [HS_FLOW] score=%d, high_score_count=%d, MAX=%d\n", 
                   score, count, 25);
            
            // Check if this is a high score
            bool is_high_score = comet_buster_is_high_score(&gui->visualizer.comet_buster, score);
            SDL_Log("[Comet Busters] [HS_FLOW] is_high_score() returned: %s\n", is_high_score ? "TRUE" : "FALSE");
            
            if (is_high_score) {
                SDL_Log("[Comet Busters] [HS_FLOW] >>> SHOWING DIALOG\n");
                // Show high score entry dialog
                hs_entry->state = HIGH_SCORE_ENTRY_ACTIVE;
                hs_entry->cursor_pos = 0;
                hs_entry->kb_selected_index = 0;  // Start at first key
                hs_entry->kb_show = true;         // Show virtual keyboard
                memset(hs_entry->name_input, 0, sizeof(hs_entry->name_input));
                SDL_Log("[Comet Busters] [HIGHSCORE] New high score! Score: %d\n", score);
            } else {
                SDL_Log("[Comet Busters] [HS_FLOW] >>> SHOWING MENU (score doesn't qualify)\n");
                // Score doesn't qualify - always go to menu
                gui->show_menu = true;
                gui->menu_state = 0;  // Main menu
                gui->menu_selection = 0;
                // Reset state so dialog can show again if next score qualifies
                hs_entry->state = HIGH_SCORE_ENTRY_NONE;
                SDL_Log("[Comet Busters] [HIGHSCORE] Game over. Score: %d (not a high score)\n", score);
                gui->visualizer.comet_buster.ship_lives = 0;
                gui->visualizer.comet_buster.game_over = true;
                
            }
        } else {
            SDL_Log("[Comet Busters] [HS_FLOW] Dialog already active, not triggering again\n");
        }
        SDL_Log("[Comet Busters] [HS_FLOW] <<< END GAME OVER HANDLING\n\n");
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
    
    // Render menu system (separated into comet_main_gl_menu.cpp)
    render_menu(gui);
    
    
    // Render help overlay if active
    if (gui->show_help_overlay) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        // Title - bright white for high contrast
        gl_set_color(1.0f, 1.0f, 1.0f);
        //gl_draw_text_simple(help_controls_text[gui->visualizer.comet_buster.current_language], 800, 50, 28);
        
        // Main help text box - large single box with all text
        int box_x = 100;
        int box_y = 130;
        int box_width = 1720;
        int box_height = 850;
        
        // Text box background
        gl_set_color(0.1f, 0.1f, 0.2f);
        gl_draw_rect_filled(box_x, box_y, box_width, box_height);
        
        // Text box border - subtle cyan
        gl_set_color(0.2f, 0.6f, 0.8f);
        gl_draw_rect_filled(box_x - 1, box_y - 1, box_width + 2, box_height + 2);
        gl_set_color(0.1f, 0.1f, 0.2f);
        gl_draw_rect_filled(box_x, box_y, box_width, box_height);
        
        // Get help text for current language
        const char* const *help_text = help_menu_text[gui->visualizer.comet_buster.current_language];
        int num_help_items = 1;
        
        // Draw all help text in the box - white text for 508 compliance
        int text_x = box_x + 40;
        int text_y = box_y + 65;
        int line_height = 56;  // Proper spacing for 14pt font with ascenders/descenders
        
        gl_set_color(1.0f, 1.0f, 1.0f);  // Pure white for maximum contrast
        for (int i = 0; i < num_help_items; i++) {
            gl_draw_text_simple(help_text[i], text_x, text_y + (i * line_height), 24);
        }
        
        // Instructions - light gray
        gl_set_color(0.9f, 0.9f, 0.9f);
        gl_draw_text_simple(hint_continue_back[gui->visualizer.comet_buster.current_language], 750, 980, 14);
    }
    
    if (hs_entry && hs_entry->state == HIGH_SCORE_ENTRY_ACTIVE) {
        extern void gl_set_color_alpha(float r, float g, float b, float a);
        extern void gl_draw_rect_filled(float x, float y, float width, float height);
        extern void gl_set_color(float r, float g, float b);
        extern void gl_draw_text_simple(const char *text, int x, int y, int font_size);
        
        // Semi-transparent overlay
        gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
        gl_draw_rect_filled(0, 0, 1920, 1080);
        
        // Dialog box background - Much larger to accommodate keyboard
        int dialog_x = 300;
        int dialog_y = 50;
        int dialog_width = 1320;
        int dialog_height = 980;
        
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
        gl_draw_text_simple(label_new_high_score[gui->visualizer.comet_buster.current_language], 740, 95, 24);
        
        // Score display - Dark brown
        gl_set_color(0.3f, 0.25f, 0.15f);
        char score_text[128];
        snprintf(score_text, sizeof(score_text), fmt_score_wave[gui->visualizer.comet_buster.current_language],  gui->visualizer.comet_buster.score, gui->visualizer.comet_buster.current_wave);
        gl_draw_text_simple(score_text, 400, 150, 14);
        
        // Name entry label - Dark brown
        gl_set_color(0.2f, 0.15f, 0.1f);
        gl_draw_text_simple(label_enter_name[gui->visualizer.comet_buster.current_language], 400, 200, 13);
        
        // Input box background - Slightly off-white
        gl_set_color(0.95f, 0.93f, 0.88f);
        gl_draw_rect_filled(400, 230, 800, 50);
        
        // Input box border - Warm gold
        gl_set_color(0.8f, 0.6f, 0.2f);
        gl_draw_rect_filled(398, 228, 804, 54);
        
        // Display the typed name - Dark brown
        gl_set_color(0.1f, 0.08f, 0.05f);  // Very dark brown
        gl_draw_text_simple(hs_entry->name_input, 415, 245, 16);
        
        // Cursor (blinking underscore)
        if ((int)(gui->total_time * 2) % 2 == 0) {  // Blink effect
            char cursor_text[] = "_";
            int cursor_x = 415 + (hs_entry->cursor_pos * 9);
            gl_draw_text_simple(cursor_text, cursor_x, 245, 16);
        }
        
        // Instructions - for keyboard
        gl_set_color(0.4f, 0.3f, 0.2f);  // Darker brown for readability
        gl_draw_text_simple(hint_name_entry2[gui->visualizer.comet_buster.current_language], 400, 310, 12);
        
        // Draw virtual keyboard
        render_virtual_keyboard(gui, hs_entry, hs_entry->kb_selected_index);
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
        gl_draw_text_simple(label_paused[gui->visualizer.comet_buster.current_language], 800, 380, 56);
        
        // Game continues in background info - Cyan
        gl_set_color(0.0f, 1.0f, 1.0f);
        gl_draw_text_simple(label_game_paused[gui->visualizer.comet_buster.current_language], 810, 480, 18);
        
        // Stats while paused - White
        gl_set_color(1.0f, 1.0f, 1.0f);
        char pause_stats[256];
        sprintf(pause_stats, fmt_pause_stats[gui->visualizer.comet_buster.current_language],
                gui->visualizer.comet_buster.current_wave,
                gui->visualizer.comet_buster.score,
                gui->visualizer.comet_buster.ship_lives);
        gl_draw_text_simple(pause_stats, 750, 530, 16);
        
        // Resume instruction - Bright cyan, larger
        gl_set_color(0.2f, 1.0f, 0.8f);
        gl_draw_text_simple(hint_resume_p[gui->visualizer.comet_buster.current_language], 795, 600, 24);
        
        // Additional hints - Dimmer cyan
        gl_set_color(0.0f, 0.8f, 0.9f);
        gl_draw_text_simple(hint_esc_menu[gui->visualizer.comet_buster.current_language], 835, 650, 16);
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
        gl_draw_text_simple(label_cheat_menu[gui->visualizer.comet_buster.current_language], 800, 230, 26);
        
        // Cheat options
        int option_y = 300;
        int line_height = 80;
        
        // Option 0: Wave
        gl_set_color(cheat_menu->selection == 0 ? 1.0f : 0.7f, 
                     cheat_menu->selection == 0 ? 1.0f : 0.7f, 
                     cheat_menu->selection == 0 ? 1.0f : 0.7f);
        char wave_text[128];
        sprintf(wave_text, fmt_cheat_wave[gui->visualizer.comet_buster.current_language], cheat_menu->wave);
        gl_draw_text_simple(wave_text, 550, option_y, 16);
        
        // Option 1: Lives
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 1 ? 1.0f : 0.7f,
                     cheat_menu->selection == 1 ? 1.0f : 0.7f,
                     cheat_menu->selection == 1 ? 1.0f : 0.7f);
        char lives_text[128];
        sprintf(lives_text, fmt_cheat_lives[gui->visualizer.comet_buster.current_language], cheat_menu->lives);
        gl_draw_text_simple(lives_text, 550, option_y, 16);
        
        // Option 2: Missiles
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 2 ? 1.0f : 0.7f,
                     cheat_menu->selection == 2 ? 1.0f : 0.7f,
                     cheat_menu->selection == 2 ? 1.0f : 0.7f);
        char missiles_text[128];
        sprintf(missiles_text, fmt_cheat_missiles[gui->visualizer.comet_buster.current_language], cheat_menu->missiles);
        gl_draw_text_simple(missiles_text, 550, option_y, 16);
        
        // Option 3: Bombs
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 3 ? 1.0f : 0.7f,
                     cheat_menu->selection == 3 ? 1.0f : 0.7f,
                     cheat_menu->selection == 3 ? 1.0f : 0.7f);
        char bombs_text[128];
        sprintf(bombs_text, fmt_cheat_bombs[gui->visualizer.comet_buster.current_language], cheat_menu->bombs);
        gl_draw_text_simple(bombs_text, 550, option_y, 16);

        // Option 4: Difficulty
        option_y += line_height;
        gl_set_color(cheat_menu->selection == 4 ? 1.0f : 0.7f,
                     cheat_menu->selection == 4 ? 1.0f : 0.7f,
                     cheat_menu->selection == 4 ? 1.0f : 0.7f);
        char difficulty_text[128];
        snprintf(difficulty_text, sizeof(difficulty_text), "%s", fmt_cheat_difficulty[gui->visualizer.comet_buster.current_language][cheat_menu->cheat_difficulty]);

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
        gl_draw_text_simple(label_apply[gui->visualizer.comet_buster.current_language], 600, option_y+5, 14);
        
        // Option 6: Cancel
        option_y += 60;
        gl_set_color(cheat_menu->selection == 6 ? 1.0f : 0.5f,
                     cheat_menu->selection == 6 ? 0.2f : 0.1f,
                     cheat_menu->selection == 6 ? 0.2f : 0.1f);
        gl_draw_rect_filled(550, option_y - 10, 200, 35);
        gl_set_color(cheat_menu->selection == 6 ? 1.0f : 0.8f,
                     cheat_menu->selection == 6 ? 1.0f : 0.8f,
                     cheat_menu->selection == 6 ? 1.0f : 0.8f);
        gl_draw_text_simple(label_cancel[gui->visualizer.comet_buster.current_language], 600, option_y+5, 14);
        
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
    SDL_Log("[Comet Busters] === Comet Busters ===\n");
    
    CometGUI gui;
    memset(&gui, 0, sizeof(CometGUI));
    
    // Explicitly initialize preferences struct to avoid junk data
    memset(&gui.preferences, 0, sizeof(CometPreferences));
    
    gui.window_width = 1024;
    gui.window_height = 768;
    gui.running = true;
    gui.finale_music_started = false;  // Initialize music state
    
    // LOAD PREFERENCES FROM DISK
    bool prefs_file_exists = preferences_load(&gui.preferences);
    gui.music_volume = gui.preferences.music_volume;
    gui.sfx_volume = gui.preferences.sfx_volume;
    
    SDL_Log("[Comet Busters] [INIT] Loaded preferences: music_volume=%d, sfx_volume=%d, language=%d\n", gui.music_volume, gui.sfx_volume, gui.preferences.language);
    
        // Load WAD file with sounds
    std::string wadPath;
    bool wad_loaded = false;

#ifdef ANDROID
    // Android: Use JNI to load WAD directly from APK assets
    // This is handled by jni_wad_loading.cpp which is initialized in SDLActivity.java
    
    SDL_Log("[Comet Busters] [AUDIO] Android: Loading WAD from APK assets via JNI...\n");
    
    // Declare the external JNI functions from jni_wad_loading.cpp
    extern unsigned char* load_wad_android(const char *wad_filename, size_t *out_size);
    extern const char* get_app_files_dir_android(void);
    
    size_t wad_size = 0;
    unsigned char* wad_data = load_wad_android("cometbuster.wad", &wad_size);
    
    if (wad_data && wad_size > 0) {
        SDL_Log("[Comet Busters] [AUDIO] [OK] WAD loaded into memory: %zu bytes\n", wad_size);
        
        // Try to save the WAD data to the app files directory for caching/backup
        const char* app_files_dir = get_app_files_dir_android();
        if (app_files_dir) {
            char full_wad_path[512];
            snprintf(full_wad_path, sizeof(full_wad_path), "%s/cometbuster.wad", app_files_dir);
            
            // Write the WAD data to the extracted location
            FILE* wad_file = fopen(full_wad_path, "wb");
            if (wad_file) {
                size_t bytes_written = fwrite(wad_data, 1, wad_size, wad_file);
                fclose(wad_file);
                
                if (bytes_written == wad_size) {
                    SDL_Log("[Comet Busters] [AUDIO] [OK] WAD cached to: %s (%zu bytes)\n", full_wad_path, wad_size);
                    if (audio_init(&gui.audio)) {
                        SDL_Log("[Comet Busters] [AUDIO] System initialized\n");
                    }

                    wad_loaded = audio_load_wad(&gui.audio, full_wad_path);
                    
                    if (wad_loaded) {
                        SDL_Log("[Comet Busters] [AUDIO] [OK] WAD loaded successfully from cached location\n");
                    } else {
                        SDL_Log("[Comet Busters] [WARNING] Failed to load WAD from cached file\n");
                    }
                } else {
                    SDL_Log("[Comet Busters] [WARNING] Incomplete write: %zu of %zu bytes\n", bytes_written, wad_size);
                }
            } else {
                SDL_Log("[Comet Busters] [WARNING] Could not open app files directory for writing\n");
            }
        }
        
        // Free the loaded WAD data
        free(wad_data);
        
    } else {
        SDL_Log("[Comet Busters] [WARNING] JNI WAD loading failed, attempting fallback methods...\n");
        
        // Fallback 1: Try extracted file location from previous run
        const char* app_files_dir = get_app_files_dir_android();
        if (app_files_dir) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/cometbuster.wad", app_files_dir);
            
            FILE* test_file = fopen(full_path, "rb");
            if (test_file) {
                fclose(test_file);
                SDL_Log("[Comet Busters] [AUDIO] Found WAD at extracted location: %s\n", full_path);
                wad_loaded = audio_load_wad(&gui.audio, full_path);
                
                if (wad_loaded) {
                    SDL_Log("[Comet Busters] [AUDIO] [OK] WAD loaded successfully from extracted location\n");
                }
            }
        }
        
        // Fallback 2: Try current directory
        if (!wad_loaded) {
            FILE* test_file = fopen("cometbuster.wad", "rb");
            if (test_file) {
                fclose(test_file);
                SDL_Log("[Comet Busters] [AUDIO] Found WAD in current directory\n");
                wad_loaded = audio_load_wad(&gui.audio, "cometbuster.wad");
                
                if (wad_loaded) {
                    SDL_Log("[Comet Busters] [AUDIO] [OK] WAD loaded successfully\n");
                }
            }
        }
        
        if (!wad_loaded) {
            SDL_Log("[Comet Busters] [ERROR] Could not load WAD from any location\n");
            SDL_Log("[Comet Busters] [HINT] Make sure cometbuster.wad exists in APK assets/\n");
            SDL_Log("[Comet Busters] [HINT] Or place it in the app files directory\n");
        }
    }

#else
    // Desktop platforms: Windows and Linux
#ifdef _WIN32
    wadPath = getExecutableDir() + "\\cometbuster.wad";
#else
    wadPath = "cometbuster.wad";
#endif

    if (!init_sdl_and_opengl(&gui, gui.window_width, gui.window_height)) {
        SDL_Log("[Comet Busters] Failed to initialize video\n");
        return 1;
    }
    
    // Window is now maximized, get the actual size
    SDL_GetWindowSize(gui.window, &gui.window_width, &gui.window_height);
    SDL_Log("[Comet Busters] [INIT] Window maximized size: %dx%d\n", gui.window_width, gui.window_height);
    
    // Initialize visualizer with GAME space (1920x1080), not window size
    memset(&gui.visualizer, 0, sizeof(Visualizer));
    gui.visualizer.width = 1920;
    gui.visualizer.height = 1080;
    gui.visualizer.mouse_x = 960;
    gui.visualizer.mouse_y = 540;
    gui.visualizer.scroll_direction = 0;  // Initialize scroll wheel state
    
    SDL_Log("[Comet Busters] [INIT] Game initialized\n");
    
    // Load high scores
    high_scores_load(&gui.visualizer.comet_buster);
    SDL_Log("[Comet Busters] [INIT] High scores loaded\n");
    
    // Initialize audio system
    memset(&gui.audio, 0, sizeof(AudioManager));
    if (audio_init(&gui.audio)) {
        SDL_Log("[Comet Busters] [AUDIO] System initialized\n");
    } else {
        SDL_Log("[Comet Busters] [WARNING] Audio init failed\n");
    }

    SDL_Log("[Comet Busters] [AUDIO] Desktop: Loading from: %s\n", wadPath.c_str());
    wad_loaded = audio_load_wad(&gui.audio, wadPath.c_str());
    
    if (wad_loaded) {
        SDL_Log("[Comet Busters] [AUDIO] WAD loaded: %s\n", wadPath.c_str());
    } else {
        SDL_Log("[Comet Busters] [WARNING] Could not load WAD: %s\n", wadPath.c_str());
    }
#endif
    
    // Copy audio to visualizer so game code can access it
    gui.visualizer.audio = gui.audio;
    
    // Apply loaded settings to audio system
    audio_set_music_volume(&gui.audio, gui.music_volume);
    audio_set_sfx_volume(&gui.audio, gui.sfx_volume);
    
    // Load background music tracks into rotation queue (for when gameplay starts)
#ifdef ExternalSound
    audio_queue_music_for_rotation(&gui.audio, "music/track1.mp3");   // Queue track 1
    audio_queue_music_for_rotation(&gui.audio, "music/track2.mp3");   // Queue track 2
    audio_queue_music_for_rotation(&gui.audio, "music/track3.mp3");   // Queue track 3
    audio_queue_music_for_rotation(&gui.audio, "music/track4.mp3");   // Queue track 4
    audio_queue_music_for_rotation(&gui.audio, "music/track5.mp3");   // Queue track 5
    audio_queue_music_for_rotation(&gui.audio, "music/track6.mp3");   // Queue track 6
    
    SDL_Log("[Comet Busters] [AUDIO] Queued %d background music tracks for rotation\n", gui.audio.music_track_count);
    
    // Set language from preferences BEFORE playing intro
    gui.visualizer.comet_buster.current_language = gui.preferences.language;
    
    // Check if preferences file didn't exist (first run)
    if (!prefs_file_exists) {
        SDL_Log("[Comet Busters] [INIT] No preferences file found - showing language menu\n");
        gui.show_menu = true;
        gui.menu_state = 4;  // Language Menu
        gui.menu_selection = 0;
    } else {
        // Play intro music during splash screen with correct language
        play_intro(&gui, gui.visualizer.comet_buster.current_language);
    }
#endif
    
    SDL_Log("[Comet Busters] [INIT] Ready to play - press WASD to move, Z to fire, ESC to open menu, P to pause\n");
    
    // Only start with splash screen if preferences file already existed
    if (prefs_file_exists) {
        SDL_Log("[Comet Busters] [INIT] Starting with splash screen and intro music...\n");
        gui.visualizer.comet_buster.splash_screen_active = true;
        comet_buster_reset_game_with_splash(&gui.visualizer.comet_buster, true, MEDIUM);
        gui.show_menu = false;
        gui.menu_state = 0;
    } else {
        SDL_Log("[Comet Busters] [INIT] Waiting for language selection...\n");
        gui.visualizer.comet_buster.splash_screen_active = false;
        gui.show_menu = true;
        gui.menu_state = 4;  // Language Menu
    }
    
    gui.gui_difficulty_level = 2;  // Medium
    gui.show_help_overlay = false;  // Initialize help overlay
    gui.help_scroll_offset = 0;     // Initialize help scroll
    gui.main_menu_scroll_offset = 0; // Initialize main menu scroll
    gui.lang_menu_scroll_offset = 0; // Initialize language menu scroll
    gui.last_joystick_axis_time = 0; // Initialize joystick throttling
    gui.last_axis_0_state = 0;      // Initialize axis state tracking
    gui.last_axis_1_state = 0;      // Initialize axis state tracking
    
    bool splash_was_active = true;  // Track splash screen state for music stop
    
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
        
        // Detect splash screen exit and stop intro music (handles ALL exit methods)
        if (splash_was_active && !gui.visualizer.comet_buster.splash_screen_active) {
            SDL_Log("[Comet Busters] [SPLASH] MAIN LOOP DETECTOR: Splash screen exited\n");
            
            // Always stop music when splash exits, regardless of what's playing
            SDL_Log("[Comet Busters] [SPLASH] MAIN LOOP: Stopping intro music...\n");
            audio_stop_music(&gui.audio);
            
            SDL_Log("[Comet Busters] [SPLASH] MAIN LOOP: Starting background music...\n");
            audio_play_random_music(&gui.audio);
            
            SDL_Log("[Comet Busters] [SPLASH] MAIN LOOP: Music transition complete\n");
            splash_was_active = false;
        } else if (!splash_was_active && gui.visualizer.comet_buster.splash_screen_active) {
            // Splash screen reactivated (shouldn't happen normally)
            SDL_Log("[Comet Busters] [SPLASH] MAIN LOOP: Splash screen reactivated\n");
            splash_was_active = true;
        }
        
        // Reset scroll wheel input after processing (for weapon changing)
        gui.visualizer.scroll_direction = 0;
        
        // Reset mouse_just_moved flag for next frame
        //gui.visualizer.mouse_just_moved = false;
        
        render_frame(&gui, &hs_entry, &cheat_menu);
        
        uint32_t elapsed = SDL_GetTicks() - current_ticks;
        if (elapsed < 16) SDL_Delay(16 - elapsed);
    }
    
    // SAVE PREFERENCES BEFORE EXITING
    gui.preferences.music_volume = gui.music_volume;
    gui.preferences.sfx_volume = gui.sfx_volume;
    gui.preferences.language = gui.visualizer.comet_buster.current_language;
    preferences_save(&gui.preferences);
    SDL_Log("[Comet Busters] [MAIN] Preferences saved at exit\n");
    
    cleanup(&gui);
    return 0;
}
