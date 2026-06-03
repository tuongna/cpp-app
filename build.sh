#!/usr/bin/env bash
# ─── Steam Clone - Full Build Script ───────────────────────────────────────
# Usage: ./build.sh [Release|Debug]
set -euo pipefail

BUILD_TYPE="${1:-Release}"
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
UI_DIR="$ROOT_DIR/app-ui"
CORE_DIR="$ROOT_DIR/app-core"
BUILD_DIR="$ROOT_DIR/build"

echo "=============================="
echo " Steam Clone Build - $BUILD_TYPE"
echo "=============================="

# ── Step 1: Build Vue/Nuxt UI (SSG) ────────────────────────────────────────
echo ""
echo "[1/2] Building Nuxt 3 UI..."
cd "$UI_DIR"
if [ ! -d "node_modules" ]; then
    echo "  Installing npm dependencies..."
    npm install
fi
npm run generate
echo "  UI built to: $UI_DIR/.output/public"

# ── Step 2: Build C++ core ─────────────────────────────────────────────────
echo ""
echo "[2/2] Building C++ core..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$CORE_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build . --config "$BUILD_TYPE" -j"$(nproc 2>/dev/null || echo 4)"

echo ""
echo "=============================="
echo " Build complete!"
echo " Executable: $BUILD_DIR/SteamClone"
echo " UI assets:  $BUILD_DIR/ui/"
echo "=============================="
