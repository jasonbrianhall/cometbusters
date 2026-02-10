#!/bin/bash

set -e

PROJECT_NAME="cometbuster"
DOCKER_IMAGE_NAME="cometbuster-android-build"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== CometBuster Android Build ===${NC}"
echo ""

if ! command -v docker &> /dev/null; then
    echo -e "${RED}ERROR: Docker is not installed${NC}"
    exit 1
fi

if [ ! -f "Makefile.gl" ] && [ ! -f "comet_main_gl.cpp" ]; then
    echo -e "${RED}ERROR: Please run this script from CometBuster project root${NC}"
    exit 1
fi

echo -e "${YELLOW}Step 1: Building Docker image...${NC}"
if ! docker image inspect "$DOCKER_IMAGE_NAME" &> /dev/null; then
    docker build -f Dockerfile.android -t "$DOCKER_IMAGE_NAME" .
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to build Docker image${NC}"
        exit 1
    fi
else
    echo "Docker image already exists"
fi

echo -e "${YELLOW}Step 2: Creating WAD file...${NC}"

if [ -f "createwad.py" ]; then
    python3 createwad.py
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ cometbuster.wad created${NC}"
    else
        echo -e "${YELLOW}Warning: WAD creation failed, proceeding anyway${NC}"
    fi
else
    echo -e "${YELLOW}Note: createwad.py not found, skipping WAD generation${NC}"
fi

echo ""
echo -e "${YELLOW}Step 3: Setting up Android project structure...${NC}"

mkdir -p android/app/src/main/java/org/cometbuster/game
mkdir -p android/app/src/main/res/values
mkdir -p android/app/src/main/res/layout
mkdir -p android/app/src/main/res/drawable
mkdir -p android/app/src/jni/src
mkdir -p android/app/src/main/assets
mkdir -p build/android

cp -f comet_main_gl.cpp wad.cpp audio_wad.cpp cometbuster_*.cpp comet_*.cpp joystick.cpp android/app/src/jni/src/ 2>/dev/null || true
cp -f miniz.c miniz_tdef.c miniz_tinfl.c miniz_zip.c android/app/src/jni/src/ 2>/dev/null || true
cp -f *.h android/app/src/jni/src/ 2>/dev/null || true

if [ -f cometbuster.wad ]; then
    cp -f cometbuster.wad android/app/src/main/assets/
    echo -e "${GREEN}✓ cometbuster.wad copied to assets${NC}"
else
    echo -e "${YELLOW}⚠ Warning: cometbuster.wad not found - game won't have sound/data${NC}"
fi

echo -e "${GREEN}✓ Source files copied${NC}"

echo -e "${YELLOW}Step 4: Creating Android configuration files...${NC}"

mkdir -p android/gradle/wrapper
cat > android/gradle/wrapper/gradle-wrapper.properties << 'EOF'
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-8.5-bin.zip
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
EOF

cat > android/gradle.properties << 'EOF'
org.gradle.daemon=false
org.gradle.parallel=true
android.useAndroidX=true
android.enableJetifier=true
EOF

cat > android/app/src/main/AndroidManifest.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    android:versionCode="1"
    android:versionName="1.0">
    <uses-sdk
        android:minSdkVersion="21"
        android:targetSdkVersion="34" />
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.VIBRATE" />
    <application
        android:allowBackup="true"
        android:label="@string/app_name"
        android:supportsRtl="true"
        android:theme="@style/AppTheme">
        <activity
            android:name="org.libsdl.app.SDLActivity"
            android:configChanges="keyboardHidden|orientation|screenSize"
            android:screenOrientation="landscape"
            android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
EOF

cat > android/app/src/main/res/values/strings.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <string name="app_name">CometBuster</string>
</resources>
EOF

cat > android/app/src/main/res/values/styles.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <style name="AppTheme" parent="android:Theme.Material.Light.DarkActionBar" />
</resources>
EOF

cat > android/app/build.gradle << 'EOF'
plugins {
    id 'com.android.application'
}

repositories {
    google()
    mavenCentral()
}

android {
    compileSdk 34
    buildToolsVersion "34.0.0"

    defaultConfig {
        applicationId "org.cometbuster.game"
        minSdk 21
        targetSdk 34
        versionCode 1
        versionName "1.0"
    }

    buildTypes {
        release {
            minifyEnabled false
            debuggable false
        }
    }

    compileOptions {
        sourceCompatibility JavaVersion.VERSION_17
        targetCompatibility JavaVersion.VERSION_17
    }

    externalNativeBuild {
        ndkBuild {
            path 'src/jni/Android.mk'
        }
    }

    namespace "org.cometbuster.game"
}

dependencies {
    implementation 'androidx.appcompat:appcompat:1.6.1'
}
EOF

cat > android/settings.gradle << 'EOF'
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

rootProject.name = "CometBuster"
include ':app'
EOF

cat > android/build.gradle << 'EOF'
plugins {
    id 'com.android.application' version '8.1.3' apply false
}

repositories {
    google()
    mavenCentral()
}
EOF

echo -e "${GREEN}✓ Configuration files created${NC}"

echo -e "${YELLOW}Step 5: Downloading SDL2...${NC}"

if [ ! -d "android/app/src/jni/SDL2" ] || [ ! -f "android/app/src/jni/SDL2/src/SDL.c" ]; then
    cd android/app/src/jni
    
    if [ -d SDL2 ]; then
        rm -rf SDL2
    fi
    
    wget -q https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz
    tar xzf SDL2-2.28.5.tar.gz
    mv SDL2-2.28.5 SDL2
    rm SDL2-2.28.5.tar.gz
    
    echo -e "${YELLOW}Downloading SDL_mixer...${NC}"

    wget -q https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz
    tar xzf SDL2_mixer-2.6.3.tar.gz
    mv SDL2_mixer-2.6.3 SDL2_mixer
    rm SDL2_mixer-2.6.3.tar.gz

    echo -e "${YELLOW}Downloading FreeType...${NC}"

    wget -q https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.gz
    tar xzf freetype-2.13.2.tar.gz
    mv freetype-2.13.2 freetype
    rm freetype-2.13.2.tar.gz

    cd ../../../../..
    echo -e "${GREEN}✓ SDL2 downloaded${NC}"
else
    echo "SDL2 already present"
fi

echo -e "${YELLOW}Step 6: Downloading prebuilt FreeType libraries...${NC}"

mkdir -p android/app/src/jni/freetype/lib/arm64-v8a
mkdir -p android/app/src/jni/freetype/lib/armeabi-v7a
mkdir -p android/app/src/jni/freetype/include

# Download prebuilt FreeType libraries for Android NDK
# These are precompiled static libraries for Android API 21+

echo "Downloading prebuilt FreeType for arm64-v8a..."
wget -q -O android/app/src/jni/freetype/lib/arm64-v8a/libfreetype.a \
    "https://github.com/LibreSprite/freetype-android-build/releases/download/v2.13.2-android/libfreetype-arm64-v8a.a" 2>/dev/null || {
    echo -e "${YELLOW}Prebuilt not available, using minimal FreeType build...${NC}"
    # Create a minimal static library config as fallback with ALL necessary modules
    mkdir -p android/app/src/jni/freetype
    cat > android/app/src/jni/freetype/Android.mk << 'MKEOF'
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := freetype

LOCAL_SRC_FILES := \
    src/base/ftbase.c \
    src/base/ftbbox.c \
    src/base/ftbitmap.c \
    src/base/ftdebug.c \
    src/base/ftglyph.c \
    src/base/ftinit.c \
    src/base/ftsystem.c \
    src/base/ftmm.c \
    src/cff/cff.c \
    src/cid/type1cid.c \
    src/psaux/psaux.c \
    src/pshinter/pshinter.c \
    src/psnames/psnames.c \
    src/raster/raster.c \
    src/sfnt/sfnt.c \
    src/smooth/smooth.c \
    src/truetype/truetype.c \
    src/type1/type1.c \
    src/type42/type42.c \
    src/bdf/bdf.c \
    src/pcf/pcf.c \
    src/pfr/pfr.c \
    src/winfonts/winfnt.c \
    src/autofit/autofit.c \
    src/gzip/ftgzip.c \
    src/lzw/ftlzw.c \
    src/sdf/sdf.c \
    src/svg/svg.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS := -DFT2_BUILD_LIBRARY
LOCAL_CPPFLAGS := -std=c++11

include $(BUILD_STATIC_LIBRARY)
MKEOF
}

if [ -f "android/app/src/jni/freetype/lib/arm64-v8a/libfreetype.a" ]; then
    echo "Downloading prebuilt FreeType for armeabi-v7a..."
    wget -q -O android/app/src/jni/freetype/lib/armeabi-v7a/libfreetype.a \
        "https://github.com/LibreSprite/freetype-android-build/releases/download/v2.13.2-android/libfreetype-armeabi-v7a.a" 2>/dev/null || true
    
    # Create Android.mk for prebuilt libraries
    cat > android/app/src/jni/freetype/Android.mk << 'MKEOF'
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := freetype
LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libfreetype.a

include $(PREBUILT_STATIC_LIBRARY)
MKEOF
    
    echo -e "${GREEN}✓ Prebuilt FreeType libraries configured${NC}"
fi

echo -e "${YELLOW}Step 7: Creating NDK build files...${NC}"

mkdir -p android/app/src/jni

cat > android/app/src/jni/Application.mk << 'EOF'
APP_PLATFORM := android-21
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_STL := c++_shared
APP_OPTIM := release
NDK_TOOLCHAIN_VERSION := clang
EOF


# Create main game Android.mk
cat > android/app/src/jni/Android.mk << 'EOF'
LOCAL_PATH := $(call my-dir)

# Save the real project path BEFORE SDL2 messes with LOCAL_PATH
PROJECT_PATH := $(LOCAL_PATH)

# --- FreeType library ---
include $(PROJECT_PATH)/freetype/Android.mk

# --- SDL2 library ---
include $(PROJECT_PATH)/SDL2/Android.mk

# --- SDL_mixer library ---
include $(PROJECT_PATH)/SDL2_mixer/Android.mk

# --- Game code ---
include $(CLEAR_VARS)

LOCAL_MODULE := main

# Restore LOCAL_PATH so src/ is relative to jni/
LOCAL_PATH := $(PROJECT_PATH)

LOCAL_SRC_FILES := \
    GL/glew.cpp \
    src/comet_main_gl.cpp \
    src/wad.cpp \
    src/audio_wad.cpp \
    src/cometbuster_spawn.cpp \
    src/cometbuster_init.cpp \
    src/cometbuster_physics.cpp \
    src/cometbuster_collision.cpp \
    src/cometbuster_boss.cpp \
    src/cometbuster_starboss.cpp \
    src/cometbuster_render_gl.cpp \
    src/cometbuster_util.cpp \
    src/cometbuster_splashscreen.cpp \
    src/joystick.cpp \
    src/cometbuster_bombs.cpp \
    src/cometbuster_bossexplosion.cpp \
    src/comet_highscores.cpp \
    src/comet_preferences.cpp \
    src/miniz.c \
    src/miniz_tdef.c \
    src/miniz_tinfl.c \
    src/miniz_zip.c

LOCAL_C_INCLUDES := \
    $(PROJECT_PATH)/src \
    $(PROJECT_PATH)/GL \
    $(PROJECT_PATH)/SDL2/include \
    $(PROJECT_PATH)/SDL2_mixer/include \
    $(PROJECT_PATH)/freetype/include \
    $(PROJECT_PATH)/GL

LOCAL_CFLAGS := -DExternalSound -DANDROID
LOCAL_CPPFLAGS := -std=c++11 -fPIC

LOCAL_STATIC_LIBRARIES := freetype
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer
LOCAL_LDLIBS := -llog -lGLESv2 -lz -landroid

include $(BUILD_SHARED_LIBRARY)
EOF

echo -e "${GREEN}✓ NDK build files created${NC}"

echo -e "${YELLOW}Step 8: Creating OpenGL stubs...${NC}"

mkdir -p android/app/src/jni/GL

cat > android/app/src/jni/GL/glew.cpp << 'EOF'
// GLEW stub for Android
int glewExperimental = 0;
EOF

cat > android/app/src/jni/GL/glew.h << 'EOF'
#ifndef __GLEW_H__
#define __GLEW_H__
#include <GLES2/gl2.h>
#define GLEW_OK 0
inline GLenum glewInit(void) { return GLEW_OK; }
extern int glewExperimental;
#define GLEW_ARB_vertex_array_object 1
#define GLEW_ARB_framebuffer_object 1
#define GLEW_ARB_texture_float 1
#endif
EOF

cat > android/app/src/jni/GL/gl.h << 'EOF'
#ifndef __GL_H__
#define __GL_H__
#include <GLES2/gl2.h>
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#define GL_PROJECTION 0x1701
#define GL_MODELVIEW 0x1700
inline void glMatrixMode(GLenum mode) { }
inline void glLoadIdentity(void) { }
inline void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar) { }
#endif
EOF

echo -e "${GREEN}✓ OpenGL stubs created${NC}"

echo -e "${YELLOW}Step 9: Cleaning build cache...${NC}"

rm -rf android/app/build 2>/dev/null || true

echo -e "${YELLOW}Step 10: Building APK with Docker...${NC}"

mkdir -p ~/.m2
mkdir -p ~/.android/sdk-cache

docker run --rm \
    -v "$(pwd)/android:/workspace/android" \
    -v "$(pwd)/cometbuster.wad:/workspace/cometbuster.wad" \
    -v "$HOME/.m2:/root/.m2" \
    -v "$HOME/.gradle:/root/.gradle" \
    -e GRADLE_USER_HOME=/root/.gradle \
    -e JAVA_TOOL_OPTIONS="-Dfile.encoding=UTF-8" \
    "$DOCKER_IMAGE_NAME" \
    bash -c "cd /workspace/android && gradle assembleRelease --no-daemon -x lint"

if [ $? -ne 0 ]; then
    echo -e "${RED}Docker build failed${NC}"
    exit 1
fi

echo -e "${YELLOW}Step 11: Finalizing APK...${NC}"

if [ -f "android/app/build/outputs/apk/release/app-release-unsigned.apk" ]; then
    cp -f "android/app/build/outputs/apk/release/app-release-unsigned.apk" "build/android/cometbuster-unsigned.apk"
    
    # Verify WAD file is in APK
    if unzip -t "build/android/cometbuster-unsigned.apk" | grep -q "assets/cometbuster.wad"; then
        echo -e "${GREEN}✓ APK created with cometbuster.wad: build/android/cometbuster-unsigned.apk${NC}"
    else
        echo -e "${YELLOW}⚠ Warning: cometbuster.wad not found in APK${NC}"
        echo -e "${YELLOW}   The game will not have audio/resources${NC}"
    fi
else
    echo -e "${RED}APK not found after build${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo "APK is ready at: build/android/cometbuster-unsigned.apk"
