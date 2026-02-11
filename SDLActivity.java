package org.libsdl.app;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;

/**
 * SDL Activity for CometBuster
 * Full SDL2 integration + Custom JNI WAD loading
 */
public class SDLActivity extends Activity {
    
    private static final String TAG = "CometBuster";
    
    // SDL2 required static variables
    public static SDLActivity mSingleton;
    public static SurfaceView mSurface;
    protected static final int SDL_ORIENTATION_PORTRAIT = 0;
    protected static final int SDL_ORIENTATION_LANDSCAPE = 1;
    protected static final int SDL_ORIENTATION_PORTRAIT_FLIPPED = 2;
    protected static final int SDL_ORIENTATION_LANDSCAPE_FLIPPED = 3;
    protected static int mCurrentOrientation = SDL_ORIENTATION_PORTRAIT;
    
    // Native state enum for SDL2
    public enum NativeState {
        INIT, RESUMED, PAUSED
    }
    public static NativeState mNextNativeState = NativeState.INIT;
    
    // Native methods required by SDL2
    public native static void nativeSetupJNI();
    public native static void initialize();
    public native static void onNativeResize();
    public native static void onNativeSurfaceCreated();
    public native static void onNativeSurfaceDestroyed();
    public native static void onNativeSurfaceChanged();
    public native static void onNativeAccel(float x, float y, float z);
    public native static void onNativeOrientationChanged(int orientation);
    public native static void onNativeMouse(int button, int action, float x, float y, boolean relative_mode);
    public native static void onNativeTouch(int touchDevId, int pointerFingerId, int action, float x, float y, float p);
    public native static void nativeSetScreenResolution(int surfaceWidth, int surfaceHeight, int nDeviceWidth, int nDeviceHeight, float density);
    public native static void handleNativeState();
    public native static boolean handleKeyEvent(android.view.View v, int keyCode, android.view.KeyEvent event, java.lang.Object unused);
    
    // Custom WAD loading JNI methods
    private native void initAssetManager(AssetManager assetManager);
    private native void setAppFilesDir(String path);
    
    // SDL motion listener
    private static SDLGenericMotionListener_API12 mMotionListener;
    
    static {
        try {
            System.loadLibrary("c++_shared");
        } catch (UnsatisfiedLinkError e) {
            Log.d(TAG, "c++_shared not found (non-critical)");
        }
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        Log.i(TAG, "=== SDLActivity.onCreate() ===");
        
        // Set singleton reference
        mSingleton = this;
        
        // Step 1: Load native libraries FIRST (before calling any native methods)
        Log.i(TAG, "Loading native libraries...");
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("main");
        Log.i(TAG, "All native libraries loaded");
        
        // Step 2: Initialize JNI asset manager for direct APK asset access
        Log.i(TAG, "Initializing JNI asset manager...");
        initAssetManager(getAssets());
        
        // Step 3: Set app files directory for JNI code to use
        Log.i(TAG, "Setting app files directory...");
        setAppFilesDir(getFilesDir().getAbsolutePath());
        
        // Step 4: Extract WAD file as fallback
        Log.i(TAG, "Extracting WAD file as fallback...");
        extractWad();
        
        // Step 5: Setup SDL JNI
        Log.i(TAG, "Setting up SDL JNI...");
        nativeSetupJNI();
        
        // Step 6: Create SDL surface
        Log.i(TAG, "Creating SDL surface...");
        mSurface = new SDLSurface(getApplication());
        setContentView(mSurface);
        
        // Step 7: Initialize SDL
        Log.i(TAG, "Initializing SDL...");
        initialize();
        
        Log.i(TAG, "=== SDLActivity.onCreate() complete ===");
    }
    
    /**
     * Extract WAD file from APK assets to app files directory
     */
    private void extractWad() {
        try {
            String wadFileName = "cometbuster.wad";
            java.io.File wadFile = new java.io.File(getFilesDir(), wadFileName);
            
            if (wadFile.exists()) {
                Log.i(TAG, "WAD file already extracted: " + wadFile.getAbsolutePath() + 
                      " (" + wadFile.length() + " bytes)");
                return;
            }
            
            Log.i(TAG, "Extracting WAD file: " + wadFileName);
            
            java.io.InputStream in = getAssets().open(wadFileName);
            java.io.FileOutputStream out = new java.io.FileOutputStream(wadFile);
            
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
        }
    }
    
    // Getters required by SDL2
    public static Context getContext() {
        return mSingleton;
    }
    
    public static SDLGenericMotionListener_API12 getMotionListener() {
        if (mMotionListener == null) {
            mMotionListener = new SDLGenericMotionListener_API12();
        }
        return mMotionListener;
    }
    
    public static SurfaceView getContentView() {
        return mSurface;
    }
    
    public static boolean isDeXMode() {
        return false;  // Not in DeX mode (Samsung DeX) - safe default
    }
}
