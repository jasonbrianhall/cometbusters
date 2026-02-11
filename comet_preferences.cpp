#include "comet_preferences.h"
#include "comet_lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#ifdef ANDROID
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

// ============================================================
// PREFERENCES FILE PATH MANAGEMENT
// ============================================================

static char preferences_path[4096] = {0};

static void build_preferences_path(void) {
    if (preferences_path[0] != '\0') {
        return;  // Already built
    }

#ifdef _WIN32
    // Windows: Use AppData\Local\CometBuster
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        snprintf(preferences_path, sizeof(preferences_path), 
                 "%s\\CometBuster", appdata);
        // Create directory if it doesn't exist
        CreateDirectoryA(preferences_path, NULL);
        snprintf(preferences_path, sizeof(preferences_path), 
                 "%s\\CometBuster\\preferences.cfg", appdata);
    } else {
        // Fallback to current directory
        strcpy(preferences_path, "preferences.cfg");
    }
#else
    // Unix/Linux: Use ~/.config/cometbuster
    const char *home = getenv("HOME");
    if (home) {
        snprintf(preferences_path, sizeof(preferences_path), 
                 "%s/.config/cometbuster", home);
        // Create directory if it doesn't exist
        mkdir(preferences_path, 0755);
        snprintf(preferences_path, sizeof(preferences_path), 
                 "%s/.config/cometbuster/preferences.cfg", home);
    } else {
        // Fallback to current directory
        strcpy(preferences_path, "preferences.cfg");
    }
#endif

    SDL_Log("[PREFS] Path: %s\n", preferences_path);
}

const char* preferences_get_path(void) {
    if (preferences_path[0] == '\0') {
        build_preferences_path();
    }
    return preferences_path;
}

// ============================================================
// PREFERENCES FUNCTIONS
// ============================================================

void preferences_init_defaults(CometPreferences *prefs) {
    if (!prefs) return;
    
    prefs->language = WLANG_ENGLISH;
    prefs->music_volume = 100;
    prefs->sfx_volume = 100;
    
    SDL_Log("[PREFS] Initialized with defaults\n");
}

bool preferences_load(CometPreferences *prefs) {
    if (!prefs) return false;
    
    // CRITICAL: Zero out the struct first to avoid junk data
    memset(prefs, 0, sizeof(CometPreferences));
    
    // Initialize with defaults
    preferences_init_defaults(prefs);
    
    const char *path = preferences_get_path();
    FILE *f = fopen(path, "rb");
    
    if (!f) {
        SDL_Log("[PREFS] No preferences file found at: %s\n", path);
        SDL_Log("[PREFS] Using default values\n");
        return false;  // File doesn't exist, use defaults
    }
    
    // Read the structure
    if (fread(prefs, sizeof(CometPreferences), 1, f) != 1) {
        SDL_Log("[PREFS] Failed to read preferences file\n");
        fclose(f);
        memset(prefs, 0, sizeof(CometPreferences));
        preferences_init_defaults(prefs);
        return false;
    }
    
    fclose(f);
    
    // Validate values - check for corruption/junk data
    if (prefs->language < 0 || prefs->language >= WLANG_COUNT) {
        SDL_Log("[PREFS] WARNING: Invalid language value (%d), resetting to default\n", prefs->language);
        prefs->language = WLANG_ENGLISH;
    }
    if (prefs->music_volume < 0 || prefs->music_volume > 100) {
        SDL_Log("[PREFS] WARNING: Invalid music volume (%d), resetting to 100\n", prefs->music_volume);
        prefs->music_volume = 100;
    }
    if (prefs->sfx_volume < 0 || prefs->sfx_volume > 100) {
        SDL_Log("[PREFS] WARNING: Invalid SFX volume (%d), resetting to 100\n", prefs->sfx_volume);
        prefs->sfx_volume = 100;
    }
    
    SDL_Log("[PREFS] Loaded preferences:\n");
    SDL_Log("[PREFS]   Language: %d\n", prefs->language);
    SDL_Log("[PREFS]   Music Volume: %d\n", prefs->music_volume);
    SDL_Log("[PREFS]   SFX Volume: %d\n", prefs->sfx_volume);
    
    return true;
}

bool preferences_save(const CometPreferences *prefs) {
    if (!prefs) return false;
    
    // Create a validated copy to save
    CometPreferences validated = *prefs;
    
    SDL_Log("[PREFS] DEBUG: About to save - struct size: %zu bytes\n", sizeof(CometPreferences));
    SDL_Log("[PREFS] DEBUG: Input language: %d, music: %d, sfx: %d\n", 
           prefs->language, prefs->music_volume, prefs->sfx_volume);
    
    // Ensure all values are valid before saving
    if (validated.language < 0 || validated.language >= WLANG_COUNT) {
        SDL_Log("[PREFS] WARNING: Invalid language (%d) during save, using default\n", validated.language);
        validated.language = WLANG_ENGLISH;
    }
    if (validated.music_volume < 0 || validated.music_volume > 100) {
        SDL_Log("[PREFS] WARNING: Invalid music volume (%d) during save, clamping to valid range\n", validated.music_volume);
        validated.music_volume = (validated.music_volume < 0) ? 0 : 100;
    }
    if (validated.sfx_volume < 0 || validated.sfx_volume > 100) {
        SDL_Log("[PREFS] WARNING: Invalid SFX volume (%d) during save, clamping to valid range\n", validated.sfx_volume);
        validated.sfx_volume = (validated.sfx_volume < 0) ? 0 : 100;
    }
    
    SDL_Log("[PREFS] DEBUG: Validated language: %d, music: %d, sfx: %d\n", 
           validated.language, validated.music_volume, validated.sfx_volume);
    
    const char *path = preferences_get_path();
    FILE *f = fopen(path, "wb");
    
    if (!f) {
        SDL_Log("[PREFS] Failed to open preferences file for writing: %s\n", path);
        return false;
    }
    
    size_t bytes_written = fwrite(&validated, sizeof(CometPreferences), 1, f);
    SDL_Log("[PREFS] DEBUG: fwrite returned: %zu (expected 1)\n", bytes_written);
    
    if (bytes_written != 1) {
        SDL_Log("[PREFS] Failed to write preferences file\n");
        fclose(f);
        return false;
    }
    
    fclose(f);
    
    SDL_Log("[PREFS] Saved preferences:\n");
    SDL_Log("[PREFS]   Language: %d\n", validated.language);
    SDL_Log("[PREFS]   Music Volume: %d\n", validated.music_volume);
    SDL_Log("[PREFS]   SFX Volume: %d\n", validated.sfx_volume);
    
    return true;
}
