package org.libsdl.app;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.content.res.AssetManager;
import android.util.Log;

/**
 * SDL Activity for CometBuster with integrated JNI WAD loading
 * Handles both direct asset loading and fallback extraction
 */
public class SDLActivity extends Activity {
    
    private static final String TAG = "CometBuster";
    
    static {
        // Load shared libraries in order
        try {
            System.loadLibrary("c++_shared");
        } catch (UnsatisfiedLinkError e) {
            Log.d(TAG, "c++_shared not found (non-critical)");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        Log.i(TAG, "SDLActivity.onCreate() starting initialization");
        
        // Step 1: Initialize JNI asset manager for direct APK asset access
        Log.i(TAG, "Initializing JNI asset manager...");
        initAssetManager(getAssets());
        
        // Step 2: Set app files directory for JNI code to use
        Log.i(TAG, "Setting app files directory...");
        setAppFilesDir(getFilesDir().getAbsolutePath());
        
        // Step 3: Extract WAD file as fallback (in case JNI loading fails)
        Log.i(TAG, "Extracting WAD file as fallback...");
        extractWad();
        
        // Step 4: Load native libraries
        Log.i(TAG, "Loading native libraries...");
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("main");
        
        Log.i(TAG, "All native libraries loaded successfully");
        
        // Step 5: Create and set surface view
        Log.i(TAG, "Creating OpenGL surface view...");
        SurfaceView surfaceView = new SurfaceView(this);
        setContentView(surfaceView);
        
        Log.i(TAG, "SDLActivity initialization complete");
    }

    /**
     * JNI: Initialize the asset manager for native code
     * This is called automatically during onCreate()
     * Allows native code to access files directly from the APK
     */
    private native void initAssetManager(AssetManager assetManager);

    /**
     * JNI: Set the app files directory path
     * Called automatically during onCreate()
     * Path: /data/data/org.cometbuster.game/files/
     */
    private native void setAppFilesDir(String path);

    /**
     * Extract WAD file from APK assets to app files directory
     * 
     * This serves as a fallback if:
     * - JNI asset loading fails
     * - Native code wants direct file access
     * 
     * Only extracts once - subsequent calls check if file exists
     */
    private void extractWad() {
        try {
            String wadFileName = "cometbuster.wad";
            java.io.File wadFile = new java.io.File(getFilesDir(), wadFileName);
            
            // Check if already extracted
            if (wadFile.exists()) {
                Log.i(TAG, "WAD file already extracted: " + wadFile.getAbsolutePath() + 
                      " (" + wadFile.length() + " bytes)");
                return;
            }
            
            Log.i(TAG, "Extracting WAD file: " + wadFileName);
            
            // Open asset from APK
            java.io.InputStream in = getAssets().open(wadFileName);
            java.io.FileOutputStream out = new java.io.FileOutputStream(wadFile);
            
            // Copy file in 64KB chunks
            byte[] buffer = new byte[65536];
            int bytesRead;
            long totalBytes = 0;
            
            while ((bytesRead = in.read(buffer)) != -1) {
                out.write(buffer, 0, bytesRead);
                totalBytes += bytesRead;
            }
            
            in.close();
            out.close();
            
            Log.i(TAG, "WAD extraction complete: " + wadFile.getAbsolutePath() + 
                  " (" + totalBytes + " bytes)");
            
        } catch (java.io.FileNotFoundException e) {
            Log.w(TAG, "WAD file not found in assets (non-critical): " + e.getMessage());
        } catch (Exception e) {
            Log.e(TAG, "Failed to extract WAD: " + e.getMessage());
            Log.e(TAG, "Stack trace: " + Log.getStackTraceString(e));
        }
    }

    /**
     * Get app files directory for potential Java-side use
     * Path: /data/data/org.cometbuster.game/files/
     */
    public String getAppFilesDir() {
        return getFilesDir().getAbsolutePath();
    }
}
