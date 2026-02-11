#ifdef ANDROID

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <jni.h>
#include <SDL.h>

static FILE* debug_log = NULL;
static char g_app_files_dir[512] = {0};  // Store the path Java tells us

static void debug_log_open() {
    if (!debug_log) {
        debug_log = fopen("/data/data/org.cometbuster.game/files/debug.log", "w");
    }
}

static void debug_log_write(const char *fmt, ...) {
    debug_log_open();
    if (debug_log) {
        va_list args;
        va_start(args, fmt);
        vfprintf(debug_log, fmt, args);
        va_end(args);
        fprintf(debug_log, "\n");
        fflush(debug_log);
    }
}

static void debug_log_close() {
    if (debug_log) {
        fclose(debug_log);
        debug_log = NULL;
    }
}

/**
 * JNI: Called from Java to set the app files directory path
 * Java passes getFilesDir().getAbsolutePath() here
 */
JNIEXPORT void JNICALL 
Java_org_libsdl_app_SDLActivity_nativeSetAppFilesDir(JNIEnv *env, jobject obj, jstring path_jstr) {
    const char *path = env->GetStringUTFChars(path_jstr, NULL);
    if (path) {
        strncpy(g_app_files_dir, path, sizeof(g_app_files_dir) - 1);
        g_app_files_dir[sizeof(g_app_files_dir) - 1] = '\0';
        SDL_Log("[JNI] App files dir set to: %s\n", g_app_files_dir);
        env->ReleaseStringUTFChars(path_jstr, path);
    }
}

/**
 * Load WAD file from the path Java set via nativeSetAppFilesDir()
 */
unsigned char* load_wad_android(const char *wad_filename, size_t *out_size) {
    
    debug_log_open();
    debug_log_write("[WAD] load_wad_android() called with filename: %s", wad_filename);
    
    if (!wad_filename || !out_size) {
        SDL_Log("[WAD] ERROR: Invalid parameters\n");
        return NULL;
    }
    
    *out_size = 0;
    
    if (g_app_files_dir[0] == '\0') {
        SDL_Log("[WAD] ERROR: App files dir not set by Java!\n");
        return NULL;
    }
    
    SDL_Log("[WAD] ========================================\n");
    SDL_Log("[WAD] Attempting to load: %s\n", wad_filename);
    SDL_Log("[WAD] ========================================\n");
    
    // Build full path using Java-provided directory
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", g_app_files_dir, wad_filename);
    
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
    
    debug_log_write("[WAD] Memory allocated");
    
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
 * Get app files directory (returns what Java set via nativeSetAppFilesDir)
 */
const char* get_app_files_dir_android(void) {
    SDL_Log("[WAD] get_app_files_dir_android() called, returning: %s", g_app_files_dir);
    return g_app_files_dir[0] != '\0' ? g_app_files_dir : NULL;
}

#endif // ANDROID
