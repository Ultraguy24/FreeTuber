#!/usr/bin/env bash
set -euo pipefail

# Usage: ./build.sh [Debug|Release] [clean]
BUILD_TYPE=${1:-Release}
DO_CLEAN=${2:-""}
BUILD_DIR=build

echo ">>> FreeTuber build script"
echo "    Build type: $BUILD_TYPE"

if [ "$DO_CLEAN" = "clean" ]; then
  echo ">>> Cleaning ${BUILD_DIR}/"
  rm -rf "$BUILD_DIR"
fi

echo ">>> Configuring in ./${BUILD_DIR}/"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo ">>> Building with $(nproc) cores"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ">>> Deploying assets and shaders"
rsync -a assets/ "$BUILD_DIR/assets/"
rsync -a shaders/ "$BUILD_DIR/shaders/"

# Ensure we have a valid Haar cascade XML
CASCADE="$BUILD_DIR/assets/haarcascade_frontalface_default.xml"
if [ ! -s "$CASCADE" ]; then
  echo ">>> Haar cascade not found or empty—downloading from OpenCV repo"
  wget -q -O "$CASCADE" \
    https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml
  echo "    Downloaded cascade, size=$(stat -c%s "$CASCADE") bytes"
fi

echo ">>> Build+deploy complete! To run:"
echo "    cd ${BUILD_DIR} && ./FreeTuber"
