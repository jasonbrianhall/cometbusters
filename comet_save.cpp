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

