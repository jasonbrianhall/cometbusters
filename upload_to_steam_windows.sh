#!/bin/bash
# =============================================================================
# CometBusters - Steam Upload Script for Windows Build (Cross-Compile)
# Usage: ./upload_to_steam_windows.sh
# Uploads Windows build compiled on Linux with Steam support
# =============================================================================

# --- CONFIGURE THESE ---
STEAM_USERNAME="booser108"
APP_ID="4428660"
DEPOT_ID="4428661"                        # Fill this in from Steamworks > SteamPipe > Depots
BUILD_DESC="CometBusters Windows build"
BUILD_DIR="$(pwd)/build/windows"
SCRIPT_DIR="$(pwd)/steam_upload"
# -----------------------

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo ""
echo "========================================"
echo "  CometBusters Steam Upload (Windows)"
echo "========================================"
echo ""

# Check depot ID is set
if [ -z "$DEPOT_ID" ]; then
    echo -e "${RED}ERROR: DEPOT_ID is not set in this script.${NC}"
    echo ""
    echo "Find it at: https://partner.steamgames.com/apps/depots/$APP_ID"
    echo "Then open this script and fill in the DEPOT_ID variable at the top."
    exit 1
fi

# Check build exists
if [ ! -f "$BUILD_DIR/cometbuster.exe" ]; then
    echo -e "${RED}ERROR: Build not found at $BUILD_DIR/cometbuster.exe${NC}"
    echo ""
    echo "Run this first:  make STEAM=1 windows"
    exit 1
fi

# Check steam_api64.dll exists
if [ ! -f "$BUILD_DIR/steam_api64.dll" ]; then
    echo -e "${RED}ERROR: steam_api64.dll not found in $BUILD_DIR${NC}"
    echo ""
    echo "You must build WITH Steam support:"
    echo "  make STEAM=1 windows"
    exit 1
fi

# Check cometbuster.wad exists
if [ ! -f "$BUILD_DIR/cometbuster.wad" ]; then
    echo -e "${RED}ERROR: cometbuster.wad not found in $BUILD_DIR${NC}"
    echo ""
    echo "Run this first:  make wad"
    exit 1
fi

# Check LICENSE.md exists
if [ ! -f "LICENSE.md" ]; then
    echo -e "${RED}ERROR: LICENSE.md not found in project root${NC}"
    echo ""
    echo "Create a LICENSE.md file in the project root directory."
    exit 1
fi

# Copy LICENSE.md as eula.txt to build directory
cp LICENSE.md "$BUILD_DIR/eula.txt"
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to copy LICENSE.md to $BUILD_DIR/eula.txt${NC}"
    exit 1
fi

cp game_actions.vdf "$BUILD_DIR/game_actions.vdf"
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to copy game_actions.vdf to $BUILD_DIR/game_actions.vdf${NC}"
    exit 1
fi


# Check steamcmd is available
if ! command -v ~/.steam/steamcmd/steamcmd.sh &> /dev/null; then
    echo -e "${RED}ERROR: steamcmd not found.${NC}"
    echo ""
    echo "Install it with:  sudo dnf install steamcmd"
    echo "Or download from: https://developer.valvesoftware.com/wiki/SteamCMD"
    exit 1
fi

echo -e "${GREEN}✓ Build found:${NC} $BUILD_DIR"
echo -e "${GREEN}✓ App ID:${NC}     $APP_ID"
echo -e "${GREEN}✓ Depot ID:${NC}   $DEPOT_ID"
echo -e "${GREEN}✓ Steam API:${NC}  steam_api64.dll"
echo -e "${GREEN}✓ EULA:${NC}       eula.txt (from LICENSE.md)"
echo ""

# Show what will be uploaded
echo "Files to upload:"
ls -lh "$BUILD_DIR"/*.exe 2>/dev/null
ls -lh "$BUILD_DIR"/*.dll 2>/dev/null
ls -lh "$BUILD_DIR"/*.wad 2>/dev/null
ls -lh "$BUILD_DIR"/eula.txt 2>/dev/null
ls -lh "$BUILD_DIR"/game_actions.vdf 2>/dev/null

echo ""

# Confirm
read -p "Upload to Steam now? (y/N): " confirm
if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "Cancelled."
    exit 0
fi

# Create steam_upload working directory
mkdir -p "$SCRIPT_DIR"
mkdir -p "$SCRIPT_DIR/output"

# Write depot build VDF
cat > "$SCRIPT_DIR/depot_$DEPOT_ID.vdf" << EOF
"DepotBuildConfig"
{
    "DepotID"       "$DEPOT_ID"
    "ContentRoot"   "$BUILD_DIR"
    "FileMapping"
    {
        "LocalPath"  "*.exe"
        "DepotPath"  "."
        "recursive"  "0"
    }
    "FileMapping"
    {
        "LocalPath"  "*.dll"
        "DepotPath"  "."
        "recursive"  "0"
    }
    "FileMapping"
    {
        "LocalPath"  "*.wad"
        "DepotPath"  "."
        "recursive"  "0"
    }
    "FileMapping"
    {
        "LocalPath"  "eula.txt"
        "DepotPath"  "."
        "recursive"  "0"
    }
}
EOF

# Write app build VDF
cat > "$SCRIPT_DIR/app_build_$APP_ID.vdf" << EOF
"appbuild"
{
    "appid"         "$APP_ID"
    "desc"          "$BUILD_DESC"
    "buildoutput"   "$SCRIPT_DIR/output"
    "contentroot"   ""
    "setlive"       ""
    "depots"
    {
        "$DEPOT_ID"  "$SCRIPT_DIR/depot_$DEPOT_ID.vdf"
    }
}
EOF

echo ""
echo "Logging in to Steam as '$STEAM_USERNAME'..."
echo -e "${YELLOW}Note: You will be prompted for your password and Steam Guard code.${NC}"
echo ""

# Run steamcmd
~/.steam/steamcmd/steamcmd.sh \
    +login "$STEAM_USERNAME" \
    +run_app_build "$SCRIPT_DIR/app_build_$APP_ID.vdf" \
    +quit

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}========================================"
    echo -e "  Upload complete!"
    echo -e "========================================${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Go to https://partner.steamgames.com/apps/builds/$APP_ID"
    echo "  2. Find your new build and set it live on the 'default' branch"
    echo "  3. Submit for review if you haven't already"
else
    echo -e "${RED}========================================"
    echo -e "  Upload failed (exit code: $EXIT_CODE)"
    echo -e "========================================${NC}"
    echo ""
    echo "Common causes:"
    echo "  - Wrong Steam Guard code (try again, codes expire quickly)"
    echo "  - Wrong password"
    echo "  - No internet connection"
    echo "  - steamcmd session expired (just run the script again)"
fi
