#ifdef ANDROID

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <SDL.h>

static char g_app_files_dir[512] = {0};  // Store the path Java tells us
static volatile int g_jni_initialized = 0;  // Flag to track JNI initialization

/**
 * JNI: Called from Java to set the app files directory path
 * Java passes getFilesDir().getAbsolutePath() here
 * MUST be extern "C" for JNI to find it
 */
extern "C" {
JNIEXPORT void JNICALL 
Java_org_libsdl_app_SDLActivity_nativeSetAppFilesDir(JNIEnv *env, jobject obj, jstring path_jstr) {
    const char *path = env->GetStringUTFChars(path_jstr, NULL);
    if (path) {
        strncpy(g_app_files_dir, path, sizeof(g_app_files_dir) - 1);
        g_app_files_dir[sizeof(g_app_files_dir) - 1] = '\0';
        SDL_Log("[Comet Busters] [JNI] App files dir set to: %s\n", g_app_files_dir);
        env->ReleaseStringUTFChars(path_jstr, path);
        
        // Signal that initialization is complete
        g_jni_initialized = 1;
    } else {
        SDL_Log("[Comet Busters] [JNI] ERROR: Failed to get string from Java!\n");
    }
}
} // extern "C" - end JNI block

/**
 * Load WAD file from the path Java set via nativeSetAppFilesDir()
 * Called from C++ code (comet_main_gl.cpp), so NOT extern "C"
 */
unsigned char* load_wad_android(const char *wad_filename, size_t *out_size) {
    
    if (!wad_filename || !out_size) {
        SDL_Log("[Comet Busters] [WAD] ERROR: Invalid parameters\n");
        return NULL;
    }
    
    *out_size = 0;
    
    // Check if JNI has been initialized
    if (!g_jni_initialized) {
        SDL_Log("[Comet Busters] [WAD] WARNING: JNI not yet initialized by Java! App files dir cannot be accessed.\n");
        SDL_Log("[Comet Busters] [WAD] This may indicate a timing issue - returning NULL for fallback handling.\n");
        return NULL;
    }
    
    if (g_app_files_dir[0] == '\0') {
        SDL_Log("[Comet Busters] [WAD] ERROR: App files dir is empty, even though JNI was called!\n");
        return NULL;
    }
    
    SDL_Log("[Comet Busters] [WAD] ========================================\n");
    SDL_Log("[Comet Busters] [WAD] Attempting to load: %s\n", wad_filename);
    SDL_Log("[Comet Busters] [WAD] App files dir: %s\n", g_app_files_dir);
    SDL_Log("[Comet Busters] [WAD] ========================================\n");
    
    // Build full path using Java-provided directory
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", g_app_files_dir, wad_filename);
    
    SDL_Log("[Comet Busters] [WAD] Trying: %s\n", full_path);
    FILE *f = fopen(full_path, "rb");
    
    if (!f) {
        SDL_Log("[Comet Busters] [WAD] Failed to open app files dir path, trying current directory\n");
        f = fopen(wad_filename, "rb");
    }
    
    if (!f) {
        SDL_Log("[Comet Busters] [WAD] FAILED: Could not load %s\n", wad_filename);
        SDL_Log("[Comet Busters] [WAD] ========================================\n");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
        SDL_Log("[Comet Busters] [WAD] ERROR: Invalid file size (%ld bytes)\n", file_size);
        fclose(f);
        return NULL;
    }

    // Allocate memory
    unsigned char *buffer = (unsigned char *)malloc(file_size);
    if (!buffer) {
        SDL_Log("[Comet Busters] [WAD] ERROR: Out of memory allocating %ld bytes\n", file_size);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);
    
    if (bytes_read != (size_t)file_size) {
        SDL_Log("[Comet Busters] [WAD] ERROR: Read only %zu of %ld bytes\n", bytes_read, file_size);
        free(buffer);
        return NULL;
    }
    
    *out_size = file_size;
    SDL_Log("[Comet Busters] [WAD] SUCCESS: Loaded %s (%ld bytes)\n", wad_filename, file_size);
    SDL_Log("[Comet Busters] [WAD] ========================================\n");
    
    return buffer;
}

/**
 * Get app files directory (returns what Java set via nativeSetAppFilesDir)
 * Returns NULL if not yet set, allowing callers to use fallbacks
 * Called from C++ code, so NOT extern "C"
 */
const char* get_app_files_dir_android(void) {
    if (!g_jni_initialized) {
        SDL_Log("[Comet Busters] [WAD] WARNING: App files dir not yet initialized by Java\n");
        return NULL;
    }
    
    if (g_app_files_dir[0] == '\0') {
        SDL_Log("[Comet Busters] [WAD] WARNING: App files dir is empty\n");
        return NULL;
    }
    
    SDL_Log("[Comet Busters] [WAD] get_app_files_dir_android() returning: %s\n", g_app_files_dir);
    return g_app_files_dir;
}

/**
 * Check if JNI initialization is complete
 * Called from C++ code, so NOT extern "C"
 */
int is_jni_initialized_android(void) {
    return g_jni_initialized;
}

#endif // ANDROID
