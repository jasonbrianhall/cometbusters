#ifdef ANDROID

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

/**
 * Load WAD file from extracted location
 * Called from C++ code - WAD was already extracted by Java SDLActivity.ensureWADExtracted()
 * 
 * Returns: pointer to allocated buffer with WAD data, or NULL on failure
 * Note: Caller must free the returned buffer
 */
unsigned char* load_wad_android(const char *wad_filename, size_t *out_size) {
    
    if (!wad_filename || !out_size) {
        SDL_Log("[WAD] ERROR: Invalid parameters\n");
        return NULL;
    }
    
    *out_size = 0;
    
    SDL_Log("[WAD] ========================================\n");
    SDL_Log("[WAD] Attempting to load: %s\n", wad_filename);
    SDL_Log("[WAD] ========================================\n");
    
    // Try standard app files directory where Java extracted it
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "/data/data/org.cometbuster.game/files/%s", wad_filename);
    
    SDL_Log("[WAD] Trying: %s\n", full_path);
    FILE *f = fopen(full_path, "rb");
    
    if (!f) {
        SDL_Log("[WAD] Failed to open, trying current directory\n");
        f = fopen(wad_filename, "rb");
    }
    
    if (!f) {
        SDL_Log("[WAD] FAILED: Could not load %s\n", wad_filename);
        SDL_Log("[WAD] ========================================\n");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
        SDL_Log("[WAD] ERROR: Invalid file size (%ld bytes)\n", file_size);
        fclose(f);
        return NULL;
    }
    
    // Allocate memory
    unsigned char *buffer = (unsigned char *)malloc(file_size);
    if (!buffer) {
        SDL_Log("[WAD] ERROR: Out of memory allocating %ld bytes\n", file_size);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);
    
    if (bytes_read != (size_t)file_size) {
        SDL_Log("[WAD] ERROR: Read only %zu of %ld bytes\n", bytes_read, file_size);
        free(buffer);
        return NULL;
    }
    
    *out_size = file_size;
    SDL_Log("[WAD] SUCCESS: Loaded %s (%ld bytes)\n", wad_filename, file_size);
    SDL_Log("[WAD] ========================================\n");
    
    return buffer;
}

/**
 * Get app files directory path (returns hardcoded path for Android)
 */
const char* get_app_files_dir_android(void) {
    return "/data/data/org.cometbuster.game/files";
}

#endif // ANDROID
