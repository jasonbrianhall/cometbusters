#ifdef ANDROID

#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

// Global state for asset access
static AAssetManager *g_asset_manager = NULL;
static char g_app_files_dir[512] = {0};

/**
 * JNI: Initialize the asset manager
 * Called automatically from SDLActivity.onCreate()
 * 
 * Function signature for Java:
 *   private native void initAssetManager(AssetManager assetManager);
 */
JNIEXPORT void JNICALL
Java_org_libsdl_app_SDLActivity_initAssetManager(
    JNIEnv *env, 
    jobject thiz, 
    jobject asset_manager) {
    
    g_asset_manager = AAssetManager_fromJava(env, asset_manager);
    
    if (g_asset_manager) {
        SDL_Log("[JNI] Asset manager initialized successfully\n");
    } else {
        SDL_Log("[JNI] ERROR: Failed to initialize asset manager\n");
    }
}

/**
 * JNI: Set the app files directory path
 * Called from SDLActivity.onCreate() to store the app directory
 * 
 * Function signature for Java:
 *   private native void setAppFilesDir(String path);
 */
JNIEXPORT void JNICALL
Java_org_libsdl_app_SDLActivity_setAppFilesDir(
    JNIEnv *env,
    jobject thiz,
    jstring path_jstr) {
    
    // FIXED: Use C++ JNI syntax - env is already a pointer
    const char *path = env->GetStringUTFChars(path_jstr, NULL);
    
    if (path) {
        strncpy(g_app_files_dir, path, sizeof(g_app_files_dir) - 1);
        g_app_files_dir[sizeof(g_app_files_dir) - 1] = '\0';
        SDL_Log("[JNI] App files directory: %s\n", g_app_files_dir);
        env->ReleaseStringUTFChars(path_jstr, path);
    } else {
        SDL_Log("[JNI] ERROR: Failed to get app files directory path\n");
    }
}

/**
 * Load WAD file directly from APK assets
 * No extraction needed - reads directly from the APK
 * 
 * Returns: pointer to allocated buffer with WAD data, or NULL on failure
 * Note: Caller must free the returned buffer
 */
static unsigned char* load_wad_from_assets(const char *filename, size_t *out_size) {
    
    if (!g_asset_manager) {
        SDL_Log("[WAD] ERROR: Asset manager not initialized\n");
        return NULL;
    }
    
    // Open asset from APK with STREAMING mode
    // STREAMING mode allows us to use AAsset_read() to read data incrementally
    AAsset *asset = AAssetManager_open(g_asset_manager, filename, AASSET_MODE_STREAMING);
    if (!asset) {
        SDL_Log("[WAD] Could not open %s from APK assets\n", filename);
        return NULL;
    }
    
    // Get file size
    off_t wad_size = AAsset_getLength(asset);
    if (wad_size <= 0) {
        SDL_Log("[WAD] ERROR: Invalid WAD size from assets (%ld bytes)\n", wad_size);
        AAsset_close(asset);
        return NULL;
    }
    
    // Allocate memory for WAD data BEFORE reading
    unsigned char *buffer = (unsigned char *)malloc(wad_size);
    if (!buffer) {
        SDL_Log("[WAD] ERROR: Out of memory allocating %ld bytes\n", wad_size);
        AAsset_close(asset);
        return NULL;
    }
    
    // Read entire file into buffer using AAsset_read()
    // This is safe and works with all asset storage types
    int bytes_read = AAsset_read(asset, buffer, wad_size);
    
    // Close asset immediately after reading (buffer is now safe)
    AAsset_close(asset);
    
    // Verify we read the complete file
    if (bytes_read != (int)wad_size) {
        SDL_Log("[WAD] ERROR: Incomplete read from assets: %d of %ld bytes\n", bytes_read, wad_size);
        free(buffer);
        return NULL;
    }
    
    *out_size = wad_size;
    SDL_Log("[WAD] Loaded from APK assets: %s (%ld bytes)\n", filename, wad_size);
    
    return buffer;
}

/**
 * Load WAD file from extracted location
 * Fallback method when direct asset loading isn't available
 * 
 * Returns: pointer to allocated buffer with WAD data, or NULL on failure
 * Note: Caller must free the returned buffer
 */
static unsigned char* load_wad_from_file(const char *filepath, size_t *out_size) {
    
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        SDL_Log("[WAD] Could not open file: %s\n", filepath);
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
    SDL_Log("[WAD] Loaded from file: %s (%ld bytes)\n", filepath, file_size);
    
    return buffer;
}

/**
 * Primary WAD loading function for Android
 * Tries multiple loading methods in order of preference:
 * 1. Direct APK asset loading (fastest, no extraction)
 * 2. Extracted file location (fallback)
 * 3. Current directory (compatibility)
 * 
 * Returns: buffer pointer if successful, NULL if all methods fail
 * Caller must free the returned buffer when done
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
    
    // Method 1: Try direct APK asset loading
    if (g_asset_manager) {
        SDL_Log("[WAD] Method 1: Trying APK assets...\n");
        unsigned char *buffer = load_wad_from_assets(wad_filename, out_size);
        if (buffer) {
            SDL_Log("[WAD] ✓ SUCCESS: Loaded from APK assets\n");
            SDL_Log("[WAD] ========================================\n");
            return buffer;
        }
    } else {
        SDL_Log("[WAD] Method 1: Asset manager not initialized\n");
    }
    
    // Method 2: Try extracted file location
    if (g_app_files_dir[0] != '\0') {
        SDL_Log("[WAD] Method 2: Trying extracted location...\n");
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_app_files_dir, wad_filename);
        
        unsigned char *buffer = load_wad_from_file(full_path, out_size);
        if (buffer) {
            SDL_Log("[WAD] ✓ SUCCESS: Loaded from extracted file\n");
            SDL_Log("[WAD] ========================================\n");
            return buffer;
        }
    } else {
        SDL_Log("[WAD] Method 2: App files directory not set\n");
    }
    
    // Method 3: Try current directory
    SDL_Log("[WAD] Method 3: Trying current directory...\n");
    unsigned char *buffer = load_wad_from_file(wad_filename, out_size);
    if (buffer) {
        SDL_Log("[WAD] ✓ SUCCESS: Loaded from current directory\n");
        SDL_Log("[WAD] ========================================\n");
        return buffer;
    }
    
    // All methods failed
    SDL_Log("[WAD] ✗ FAILED: Could not load %s from any location\n", wad_filename);
    SDL_Log("[WAD] ========================================\n");
    return NULL;
}

/**
 * Get app files directory path (for C++ code to access)
 */
const char* get_app_files_dir_android(void) {
    return (g_app_files_dir[0] != '\0') ? g_app_files_dir : NULL;
}

#endif // ANDROID
