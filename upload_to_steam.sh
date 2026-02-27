#!/bin/bash
# =============================================================================
# CometBusters - Steam Upload Script
# Usage: ./upload_to_steam.sh
# =============================================================================

# --- CONFIGURE THESE ---
STEAM_USERNAME="booser108"
APP_ID="4428660"
DEPOT_ID="4428661"                        # Fill this in from Steamworks > SteamPipe > Depots
BUILD_DESC="CometBusters Linux build"
BUILD_DIR="$(pwd)/build/linux-static"
SCRIPT_DIR="$(pwd)/steam_upload"
# -----------------------

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo ""
echo "========================================"
echo "  CometBusters Steam Upload"
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
if [ ! -f "$BUILD_DIR/cometbuster-static" ]; then
    echo -e "${RED}ERROR: Build not found at $BUILD_DIR/cometbuster-static${NC}"
    echo ""
    echo "Run this first:  make STEAM=1 linux-static"
    exit 1
fi

# Check libsteam_api.so is present
if [ ! -f "$BUILD_DIR/libsteam_api.so" ]; then
    echo -e "${RED}ERROR: libsteam_api.so not found in $BUILD_DIR${NC}"
    echo ""
    echo "Run this first:  make STEAM=1 linux-static"
    exit 1
fi

# Check steamcmd is available
if ! command -v steamcmd &> /dev/null; then
    echo -e "${RED}ERROR: steamcmd not found.${NC}"
    echo ""
    echo "Install it with:  sudo dnf install steamcmd"
    echo "Or download from: https://developer.valvesoftware.com/wiki/SteamCMD"
    exit 1
fi

echo -e "${GREEN}✓ Build found:${NC} $BUILD_DIR"
echo -e "${GREEN}✓ App ID:${NC}     $APP_ID"
echo -e "${GREEN}✓ Depot ID:${NC}   $DEPOT_ID"
echo ""

# Show what will be uploaded
echo "Files to upload:"
ls -lh "$BUILD_DIR/cometbuster-static" "$BUILD_DIR/libsteam_api.so" "$BUILD_DIR/cometbuster.wad" 2>/dev/null
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
        "LocalPath"  "*"
        "DepotPath"  "."
        "recursive"  "1"
    }
    "FileExclusion"  "*.o"
    "FileExclusion"  "*.d"
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
steamcmd \
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
