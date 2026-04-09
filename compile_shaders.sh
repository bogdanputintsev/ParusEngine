#!/bin/bash
set -e

# Change to the directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Base directory
PROJECT_DIR="$PWD"

# Input shaders source code
SOURCE_SHADERS_DIR="$PROJECT_DIR/source/shaders"

# Output shaders directory
SHADERS_OUTPUT_DIR="$PROJECT_DIR/bin/shaders"
BUILD_DIR="build"

# Error checking
if [ -z "$VULKAN_SDK" ]; then
    echo "[ERROR] VULKAN_SDK environment variable is not set. Exiting."
    exit 1
fi

if [ ! -d "$SOURCE_SHADERS_DIR" ]; then
    echo "[ERROR] Shader source directory \"$SOURCE_SHADERS_DIR\" does not exist. Exiting."
    exit 1
fi

if [ ! -d "$SHADERS_OUTPUT_DIR" ]; then
    echo "[INFO] Creating directory for shaders output \"$SHADERS_OUTPUT_DIR\"..."
    mkdir -p "$SHADERS_OUTPUT_DIR"
fi

# Compile all shader files
FOUND_SHADERS=0
for INPUT_SHADER in "$SOURCE_SHADERS_DIR"/*.vert "$SOURCE_SHADERS_DIR"/*.frag "$SOURCE_SHADERS_DIR"/*.comp "$SOURCE_SHADERS_DIR"/*.geom "$SOURCE_SHADERS_DIR"/*.tesc "$SOURCE_SHADERS_DIR"/*.tese; do
    # Skip if glob didn't match anything
    if [ ! -f "$INPUT_SHADER" ]; then
        continue
    fi

    FOUND_SHADERS=1
    FILENAME="$(basename "$INPUT_SHADER")"
    OUTPUT_SHADER="$SHADERS_OUTPUT_DIR/$FILENAME.spv"

    echo "[INFO] Compiling shader file \"$INPUT_SHADER\"..."
    "$VULKAN_SDK/bin/glslc" "$INPUT_SHADER" -o "$OUTPUT_SHADER"
done

if [ "$FOUND_SHADERS" -eq 0 ]; then
    echo "[WARNING] No shader files found in \"$SOURCE_SHADERS_DIR\"."
fi

echo "[INFO] Shaders compiled successfully to \"$SHADERS_OUTPUT_DIR\"."

# Copy bin folder to all subdirectories of build
if [ ! -d "$BUILD_DIR" ]; then
    echo "[WARNING] Build directory \"$BUILD_DIR\" not found. Skipping bin copy step."
else
    for TARGET_DIR in "$BUILD_DIR"/*/; do
        if [ ! -d "$TARGET_DIR" ]; then
            continue
        fi

        TARGET_BIN="$TARGET_DIR/bin"

        # Remove old bin folder if it exists
        if [ -d "$TARGET_BIN" ]; then
            echo "[INFO] Removing existing bin folder in \"$TARGET_DIR\"..."
            rm -rf "$TARGET_BIN"
        fi

        echo "[INFO] Copying bin to \"$TARGET_DIR\"..."
        cp -r "$PROJECT_DIR/bin" "$TARGET_BIN"
    done
fi

echo "[INFO] Bin folder successfully copied to all build targets."