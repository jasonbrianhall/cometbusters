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

#ifdef STEAM
    #include "steam/steam_api.h"
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
 * Gets the Steam Cloud filename for a save state slot
 */
#ifdef STEAM
static void get_steam_cloud_filename(int slot, char *filename, int max_len) {
#ifndef GLSave
    snprintf(filename, max_len, "comet_state_%d.sav", slot);
#else
    snprintf(filename, max_len, "comet_stategl_%d.sav", slot);
#endif
}
#endif /* STEAM */

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

// ============================================================
// STEAM CLOUD HELPERS
// ============================================================

#ifdef STEAM

/**
 * Writes a save buffer to Steam Cloud storage.
 * Returns 1 on success, 0 on failure.
 */
static int steam_cloud_write(int slot, const void *data, size_t size) {
    if (!SteamRemoteStorage()) {
        SDL_Log("[Comet Busters] [STEAM CLOUD] ISteamRemoteStorage not available\n");
        return 0;
    }

    char cloud_filename[64];
    get_steam_cloud_filename(slot, cloud_filename, sizeof(cloud_filename));

    bool ok = SteamRemoteStorage()->FileWrite(cloud_filename, data, (int32)size);
    SDL_Log("[Comet Busters] [STEAM CLOUD] Write slot %d (%s): %s\n",
            slot, cloud_filename, ok ? "OK" : "FAILED");
    return ok ? 1 : 0;
}

/**
 * Reads a save buffer from Steam Cloud storage.
 * Returns 1 on success, 0 on failure or file not found.
 */
static int steam_cloud_read(int slot, void *data, size_t size) {
    if (!SteamRemoteStorage()) {
        SDL_Log("[Comet Busters] [STEAM CLOUD] ISteamRemoteStorage not available\n");
        return 0;
    }

    char cloud_filename[64];
    get_steam_cloud_filename(slot, cloud_filename, sizeof(cloud_filename));

    if (!SteamRemoteStorage()->FileExists(cloud_filename)) {
        SDL_Log("[Comet Busters] [STEAM CLOUD] File not found in cloud: %s\n", cloud_filename);
        return 0;
    }

    int32 bytes_read = SteamRemoteStorage()->FileRead(cloud_filename, data, (int32)size);
    SDL_Log("[Comet Busters] [STEAM CLOUD] Read slot %d (%s): %d bytes\n",
            slot, cloud_filename, bytes_read);
    return (bytes_read == (int32)size) ? 1 : 0;
}

#endif /* STEAM */

// ============================================================
// SAVE / LOAD STATE
// ============================================================

/**
 * Saves the current game state to a slot (0-10)
 */
int menu_save_state(CometGUI *gui, int slot) {
    SDL_Log("[Comet Busters] [SAVE STATE] saving to slot: %i\n", slot);

    if (slot < 0 || slot > 10) return 0;

    int version = 1;
    time_t now = time(NULL);
    CometBusterGame *game = &gui->visualizer.comet_buster;

    // ---- Build a flat buffer (header + game data) ----
    size_t buf_size = sizeof(int) + sizeof(time_t) + sizeof(CometBusterGame);
    uint8_t *buf = (uint8_t*)malloc(buf_size);
    if (!buf) {
        SDL_Log("[Comet Busters] [SAVE STATE] Out of memory\n");
        return 0;
    }
    size_t offset = 0;
    memcpy(buf + offset, &version, sizeof(int));          offset += sizeof(int);
    memcpy(buf + offset, &now,     sizeof(time_t));       offset += sizeof(time_t);
    memcpy(buf + offset, game,     sizeof(CometBusterGame));

    // ---- Write local file ----
    ensure_save_dir();

    char filename[256];
    get_state_filename(slot, filename, sizeof(filename));

    FILE *file = fopen(filename, "wb");
    if (!file) {
        SDL_Log("[Comet Busters] [SAVE STATE] Failed to open file: %s\n", filename);
        free(buf);
        return 0;
    }

    if (fwrite(buf, 1, buf_size, file) != buf_size) {
        SDL_Log("[Comet Busters] [SAVE STATE] Failed to write game state\n");
        fclose(file);
        free(buf);
        return 0;
    }
    fclose(file);
    SDL_Log("[Comet Busters] [SAVE STATE] State %d saved to %s (%zu bytes)\n",
            slot, filename, buf_size);

#ifdef STEAM
    // ---- Mirror to Steam Cloud ----
    steam_cloud_write(slot, buf, buf_size);
#endif /* STEAM */

    free(buf);
    return 1;
}

/**
 * Loads a game state from a slot (0-10)
 */
int menu_load_state(CometGUI *gui, int slot) {
    SDL_Log("[Comet Busters] [LOAD STATE] loading from slot: %i\n", slot);

    if (slot < 0 || slot > 10) return 0;

    CometBusterGame *game = &gui->visualizer.comet_buster;
    int saved_language = game->current_language;

    size_t buf_size = sizeof(int) + sizeof(time_t) + sizeof(CometBusterGame);
    uint8_t *buf = (uint8_t*)malloc(buf_size);
    if (!buf) {
        SDL_Log("[Comet Busters] [LOAD STATE] Out of memory\n");
        return 0;
    }

    int loaded = 0;

#ifdef STEAM
    // ---- Try Steam Cloud first ----
    if (steam_cloud_read(slot, buf, buf_size)) {
        int version;
        memcpy(&version, buf, sizeof(int));
        if (version == 1) {
            memcpy(game, buf + sizeof(int) + sizeof(time_t), sizeof(CometBusterGame));
            game->current_language = saved_language;
            SDL_Log("[Comet Busters] [LOAD STATE] State %d loaded from Steam Cloud (%zu bytes)\n",
                    slot, buf_size);
            loaded = 1;
        } else {
            SDL_Log("[Comet Busters] [LOAD STATE] Unsupported cloud save version: %d\n", version);
        }
    }
#endif /* STEAM */

    // ---- Fall back to local file ----
    if (!loaded) {
        char filename[256];
        get_state_filename(slot, filename, sizeof(filename));

        FILE *file = fopen(filename, "rb");
        if (!file) {
            SDL_Log("[Comet Busters] [LOAD STATE] Failed to open file: %s\n", filename);
            free(buf);
            return 0;
        }

        int version;
        time_t timestamp;

        if (fread(&version, sizeof(int), 1, file) != 1) {
            SDL_Log("[Comet Busters] [LOAD STATE] Failed to read version\n");
            fclose(file);
            free(buf);
            return 0;
        }

        if (version != 1) {
            SDL_Log("[Comet Busters] [LOAD STATE] Unsupported save version: %d\n", version);
            fclose(file);
            free(buf);
            return 0;
        }

        if (fread(&timestamp, sizeof(time_t), 1, file) != 1) {
            SDL_Log("[Comet Busters] [LOAD STATE] Failed to read timestamp\n");
            fclose(file);
            free(buf);
            return 0;
        }

        if (fread(game, sizeof(CometBusterGame), 1, file) != 1) {
            SDL_Log("[Comet Busters] [LOAD STATE] Failed to read game state\n");
            fclose(file);
            free(buf);
            return 0;
        }

        game->current_language = saved_language;
        fclose(file);
        SDL_Log("[Comet Busters] [LOAD STATE] State %d loaded from %s (%zu bytes)\n",
                slot, filename, sizeof(CometBusterGame) + sizeof(int) + sizeof(time_t));
        loaded = 1;
    }

    free(buf);
    return loaded;
}

/**
 * Checks if a save state exists (local or Steam Cloud)
 */
int menu_state_exists(int slot) {
    if (slot < 0 || slot > 9) return 0;

#ifdef STEAM
    // Check Steam Cloud first
    if (SteamRemoteStorage()) {
        char cloud_filename[64];
        get_steam_cloud_filename(slot, cloud_filename, sizeof(cloud_filename));
        if (SteamRemoteStorage()->FileExists(cloud_filename)) {
            return 1;
        }
    }
#endif /* STEAM */

    // Check local file
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

    int version;
    time_t timestamp;
    CometBusterGame game;
    int read_ok = 0;

#ifdef STEAM
    // Try reading header + game from Steam Cloud
    if (SteamRemoteStorage()) {
        char cloud_filename[64];
        get_steam_cloud_filename(slot, cloud_filename, sizeof(cloud_filename));

        size_t buf_size = sizeof(int) + sizeof(time_t) + sizeof(CometBusterGame);
        uint8_t *buf = (uint8_t*)malloc(buf_size);
        if (buf) {
            int32 bytes_read = SteamRemoteStorage()->FileRead(cloud_filename, buf, (int32)buf_size);
            if (bytes_read == (int32)buf_size) {
                memcpy(&version,   buf,                                        sizeof(int));
                memcpy(&timestamp, buf + sizeof(int),                          sizeof(time_t));
                memcpy(&game,      buf + sizeof(int) + sizeof(time_t),         sizeof(CometBusterGame));
                read_ok = (version == 1);
            }
            free(buf);
        }
    }
#endif /* STEAM */

    // Fall back to local file
    if (!read_ok) {
        char filename[256];
        get_state_filename(slot, filename, sizeof(filename));

        FILE *file = fopen(filename, "rb");
        if (!file) return "Error";

        if (fread(&version,   sizeof(int),           1, file) != 1 ||
            fread(&timestamp, sizeof(time_t),         1, file) != 1 ||
            fread(&game,      sizeof(CometBusterGame),1, file) != 1) {
            fclose(file);
            return "Error";
        }
        fclose(file);
        read_ok = 1;
    }

    if (!read_ok) return "Error";

    // Format: "Wave X | Score Y | HH:MM"
    struct tm *timeinfo = localtime(&timestamp);
    snprintf(state_info_buffer, sizeof(state_info_buffer),
             "Wave %d | Score %d | %02d:%02d",
             game.current_wave, game.score, timeinfo->tm_hour, timeinfo->tm_min);

    return state_info_buffer;
}
