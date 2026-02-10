#ifdef ANDROID

#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        fprintf(stderr, "[JNI] Asset manager initialized successfully\n");
    } else {
        fprintf(stderr, "[JNI] ERROR: Failed to initialize asset manager\n");
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
    
    const char *path = (*env)->GetStringUTFChars(env, path_jstr, NULL);
    
    if (path) {
        strncpy(g_app_files_dir, path, sizeof(g_app_files_dir) - 1);
        g_app_files_dir[sizeof(g_app_files_dir) - 1] = '\0';
        fprintf(stderr, "[JNI] App files directory: %s\n", g_app_files_dir);
        (*env)->ReleaseStringUTFChars(env, path_jstr, path);
    } else {
        fprintf(stderr, "[JNI] ERROR: Failed to get app files directory path\n");
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
        fprintf(stderr, "[WAD] ERROR: Asset manager not initialized\n");
        return NULL;
    }
    
    // Open asset from APK
    AAsset *asset = AAssetManager_open(g_asset_manager, filename, AASSET_MODE_BUFFER);
    if (!asset) {
        fprintf(stderr, "[WAD] Could not open %s from APK assets\n", filename);
        return NULL;
    }
    
    // Get file size
    off_t wad_size = AAsset_getLength(asset);
    if (wad_size <= 0) {
        fprintf(stderr, "[WAD] ERROR: Invalid WAD size from assets (%ld bytes)\n", wad_size);
        AAsset_close(asset);
        return NULL;
    }
    
    // Get file data pointer
    const unsigned char *wad_data = (const unsigned char *)AAsset_getBuffer(asset);
    if (!wad_data) {
        fprintf(stderr, "[WAD] ERROR: Failed to read WAD buffer from assets\n");
        AAsset_close(asset);
        return NULL;
    }
    
    // Allocate memory for WAD data
    unsigned char *buffer = (unsigned char *)malloc(wad_size);
    if (!buffer) {
        fprintf(stderr, "[WAD] ERROR: Out of memory allocating %ld bytes\n", wad_size);
        AAsset_close(asset);
        return NULL;
    }
    
    // Copy data
    memcpy(buffer, wad_data, wad_size);
    
    // Close asset (the buffer we allocated is now independent)
    AAsset_close(asset);
    
    *out_size = wad_size;
    fprintf(stderr, "[WAD] Loaded from APK assets: %s (%ld bytes)\n", filename, wad_size);
    
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
        fprintf(stderr, "[WAD] Could not open file: %s\n", filepath);
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fprintf(stderr, "[WAD] ERROR: Invalid file size (%ld bytes)\n", file_size);
        fclose(f);
        return NULL;
    }
    
    // Allocate memory
    unsigned char *buffer = (unsigned char *)malloc(file_size);
    if (!buffer) {
        fprintf(stderr, "[WAD] ERROR: Out of memory allocating %ld bytes\n", file_size);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "[WAD] ERROR: Read only %zu of %ld bytes\n", bytes_read, file_size);
        free(buffer);
        return NULL;
    }
    
    *out_size = file_size;
    fprintf(stderr, "[WAD] Loaded from file: %s (%ld bytes)\n", filepath, file_size);
    
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
        fprintf(stderr, "[WAD] ERROR: Invalid parameters\n");
        return NULL;
    }
    
    *out_size = 0;
    
    fprintf(stderr, "[WAD] ========================================\n");
    fprintf(stderr, "[WAD] Attempting to load: %s\n", wad_filename);
    fprintf(stderr, "[WAD] ========================================\n");
    
    // Method 1: Try direct APK asset loading
    if (g_asset_manager) {
        fprintf(stderr, "[WAD] Method 1: Trying APK assets...\n");
        unsigned char *buffer = load_wad_from_assets(wad_filename, out_size);
        if (buffer) {
            fprintf(stderr, "[WAD] ✓ SUCCESS: Loaded from APK assets\n");
            fprintf(stderr, "[WAD] ========================================\n");
            return buffer;
        }
    } else {
        fprintf(stderr, "[WAD] Method 1: Asset manager not initialized\n");
    }
    
    // Method 2: Try extracted file location
    if (g_app_files_dir[0] != '\0') {
        fprintf(stderr, "[WAD] Method 2: Trying extracted location...\n");
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_app_files_dir, wad_filename);
        
        unsigned char *buffer = load_wad_from_file(full_path, out_size);
        if (buffer) {
            fprintf(stderr, "[WAD] ✓ SUCCESS: Loaded from extracted file\n");
            fprintf(stderr, "[WAD] ========================================\n");
            return buffer;
        }
    } else {
        fprintf(stderr, "[WAD] Method 2: App files directory not set\n");
    }
    
    // Method 3: Try current directory
    fprintf(stderr, "[WAD] Method 3: Trying current directory...\n");
    unsigned char *buffer = load_wad_from_file(wad_filename, out_size);
    if (buffer) {
        fprintf(stderr, "[WAD] ✓ SUCCESS: Loaded from current directory\n");
        fprintf(stderr, "[WAD] ========================================\n");
        return buffer;
    }
    
    // All methods failed
    fprintf(stderr, "[WAD] ✗ FAILED: Could not load %s from any location\n", wad_filename);
    fprintf(stderr, "[WAD] ========================================\n");
    return NULL;
}

/**
 * Get app files directory path (for C++ code to access)
 */
const char* get_app_files_dir_android(void) {
    return (g_app_files_dir[0] != '\0') ? g_app_files_dir : NULL;
}

#endif // ANDROID
