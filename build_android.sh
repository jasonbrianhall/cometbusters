#!/bin/bash

set -e

PROJECT_NAME="cometbuster"
DOCKER_IMAGE_NAME="cometbuster-android-build"
OUTPUT_APK="app-release-unsigned.apk"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== CometBuster Android Build System ===${NC}"
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo -e "${RED}ERROR: Docker is not installed${NC}"
    echo "Please install Docker from https://docs.docker.com/get-docker/"
    exit 1
fi

# Check if we're in the right directory
if [ ! -f "Makefile.gl" ] && [ ! -f "comet_main_gl.cpp" ]; then
    echo -e "${RED}ERROR: Please run this script from your CometBuster project root${NC}"
    echo "The script looks for Makefile.gl and comet_main_gl.cpp"
    exit 1
fi

# Step 1: Build Docker image
echo -e "${YELLOW}Step 1: Building Docker image...${NC}"
if ! docker image inspect "$DOCKER_IMAGE_NAME" &> /dev/null; then
    echo "Building Docker image (this may take a few minutes)..."
    docker build -f Dockerfile.android -t "$DOCKER_IMAGE_NAME" .
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to build Docker image${NC}"
        exit 1
    fi
else
    echo "Docker image already exists, skipping build"
fi

# Step 2: Create Android project structure
echo -e "${YELLOW}Step 2: Setting up Android project structure...${NC}"

# Create all necessary directories - use single mkdir for clarity
mkdir -p android/app/src/main/java/org/cometbuster/game
mkdir -p android/app/src/main/res/values
mkdir -p android/app/src/main/res/layout
mkdir -p android/app/src/main/res/drawable
mkdir -p android/app/src/main/jni/src
mkdir -p android/app/src/main/jni/SDL2
mkdir -p android/app/src/main/assets
mkdir -p build/android

# Verify directories were created
if [ ! -d "android/app/src/main/jni" ]; then
    echo -e "${RED}ERROR: Failed to create android/app/src/main/jni directory${NC}"
    exit 1
fi

# Copy game source code
echo "Copying game source to Android project..."
cp -f comet_main_gl.cpp wad.cpp audio_wad.cpp cometbuster_*.cpp comet_*.cpp \
   joystick.cpp android/app/src/main/jni/src/ 2>/dev/null || true

cp -f miniz.c miniz_tdef.c miniz_tinfl.c miniz_zip.c android/app/src/main/jni/src/ 2>/dev/null || true

# Copy header files
cp -f *.h android/app/src/main/jni/src/ 2>/dev/null || true

# Copy WAD file if it exists
if [ -f cometbuster.wad ]; then
    cp -f cometbuster.wad android/app/src/main/assets/
fi

echo -e "${GREEN}✓ Source files copied${NC}"

# Step 3: Create Android configuration files
echo -e "${YELLOW}Step 3: Creating Android configuration files...${NC}"

# Create gradle wrapper configuration
mkdir -p android/gradle/wrapper
cat > android/gradle/wrapper/gradle-wrapper.properties << 'EOF'
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-8.5-bin.zip
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
EOF

# Create a gradle.properties to configure Gradle properly
cat > android/gradle.properties << 'EOF'
org.gradle.daemon=false
org.gradle.parallel=true
EOF

# AndroidManifest.xml
cat > android/app/src/main/AndroidManifest.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="org.cometbuster.game"
    android:versionCode="1"
    android:versionName="1.0">

    <uses-sdk
        android:minSdkVersion="21"
        android:targetSdkVersion="34" />

    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.VIBRATE" />

    <application
        android:allowBackup="true"
        android:icon="@drawable/ic_launcher"
        android:label="@string/app_name"
        android:roundIcon="@drawable/ic_launcher_round"
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

# strings.xml
cat > android/app/src/main/res/values/strings.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <string name="app_name">CometBuster</string>
</resources>
EOF

# styles.xml
cat > android/app/src/main/res/values/styles.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <style name="AppTheme" parent="android:Theme.Material.Light.DarkActionBar" />
</resources>
EOF

# build.gradle (app)
cat > android/app/build.gradle << 'EOF'
plugins {
    id 'com.android.application'
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
            path 'src/main/jni/Android.mk'
        }
    }

    namespace "org.cometbuster.game"
}

dependencies {
    implementation 'androidx.appcompat:appcompat:1.6.1'
}
EOF

# settings.gradle
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

# build.gradle (root)
cat > android/build.gradle << 'EOF'
plugins {
    id 'com.android.application' version '8.1.3' apply false
}
EOF

echo -e "${GREEN}✓ Configuration files created${NC}"

# Step 4: Create Android.mk and Application.mk
echo -e "${YELLOW}Step 4: Creating NDK build files...${NC}"

cat > android/app/src/main/jni/Application.mk << 'EOF'
APP_PLATFORM := android-21
APP_ABI := armeabi-v7a arm64-v8a
APP_STL := c++_shared
APP_OPTIM := release
NDK_TOOLCHAIN_VERSION := clang
EOF

cat > android/app/src/main/jni/Android.mk << 'EOF'
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                   $(LOCAL_PATH)/../SDL2/include

LOCAL_SRC_FILES := src/comet_main_gl.cpp \
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

LOCAL_CFLAGS := -DExternalSound -DANDROID -std=c++11 -Wall -Wextra
LOCAL_CPPFLAGS := -std=c++11
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer
LOCAL_LDLIBS := -llog -lGLESv2 -lz -landroid

include $(BUILD_SHARED_LIBRARY)

# Include SDL2 library
$(call import-module,SDL2)
EOF

echo -e "${GREEN}✓ NDK build files created${NC}"

# Step 5: Download SDL2 for Android
echo -e "${YELLOW}Step 5: Setting up SDL2 for Android...${NC}"

if [ ! -d "android/app/src/main/jni/SDL2" ] || [ ! -f "android/app/src/main/jni/SDL2/Android.mk" ]; then
    echo "Downloading SDL2..."
    cd android/app/src/main/jni
    
    if [ -d SDL2 ]; then
        rm -rf SDL2
    fi
    
    wget -q https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz
    tar xzf SDL2-2.28.5.tar.gz
    mv SDL2-2.28.5 SDL2
    rm SDL2-2.28.5.tar.gz
    
    cd ../../../../..
    echo -e "${GREEN}✓ SDL2 set up${NC}"
else
    echo "SDL2 already present"
fi

# Step 6: Run Docker build
echo -e "${YELLOW}Step 6: Building APK with Docker...${NC}"

# Create cache directories if they don't exist
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
    bash -c "cd /workspace/android && gradle --version && gradle assembleRelease --no-daemon"

if [ $? -ne 0 ]; then
    echo -e "${RED}Docker build failed${NC}"
    exit 1
fi

# Step 7: Locate and copy APK
echo -e "${YELLOW}Step 7: Finalizing APK...${NC}"

if [ -f "android/app/build/outputs/apk/release/app-release-unsigned.apk" ]; then
    cp -f "android/app/build/outputs/apk/release/app-release-unsigned.apk" "build/android/cometbuster-unsigned.apk"
    echo -e "${GREEN}✓ APK created: build/android/cometbuster-unsigned.apk${NC}"
else
    echo -e "${RED}APK not found after build${NC}"
    echo "Build output directory structure:"
    find android/app/build -type f -name "*.apk" 2>/dev/null || echo "No APK files found"
    exit 1
fi

echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Your APK is ready at: build/android/cometbuster-unsigned.apk"
echo ""
echo "To sign and deploy:"
echo "  1. Sign the APK with your key"
echo "  2. Install on Android device: adb install build/android/cometbuster-unsigned.apk"
echo ""
echo "Note: This is an unsigned APK. For Google Play Store, you'll need to sign it."
