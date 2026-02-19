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
    
#ifndef GLSave
    snprintf(filename, max_len, "%s/comet_state_%d.sav", expanded_dir, slot);
#else
    snprintf(filename, max_len, "%s/comet_stategl_%d.sav", expanded_dir, slot);
#endif
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
    
#ifndef GLSave
    snprintf(filename, max_len, "%s/comet_state_10.sav", expanded_dir);
#else
    snprintf(filename, max_len, "%s/comet_stategl_10.sav", expanded_dir);
#endif

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


