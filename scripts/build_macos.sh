#!/bin/bash
# Build psdv-film gate emulation for macOS (universal arm64 + x86_64, Metal).
# Requires: Xcode Command Line Tools (xcode-select --install) and CMake >= 3.20
#           (brew install cmake). Internet access is needed the first time to
#           download the OpenFX SDK, unless OPENFX_DIR points to a local checkout.
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-build-macos}"
CMAKE_ARGS=(-DCMAKE_BUILD_TYPE=Release -DPSDV_BUILD_TESTS=OFF
            "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0)
if [ -n "${OPENFX_DIR:-}" ]; then CMAKE_ARGS+=("-DOPENFX_DIR=$OPENFX_DIR"); fi
cmake -S . -B "$BUILD_DIR" "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR" --config Release -j "$(sysctl -n hw.ncpu)"
BUNDLE="$BUILD_DIR/psdv-film-gate.ofx.bundle"
BIN="$BUNDLE/Contents/MacOS/psdv-film-gate.ofx"
echo "--- architectures:"; lipo -info "$BIN"
echo "--- exported symbols (must be OfxGetNumberOfPlugins / OfxGetPlugin):"
nm -gU "$BIN" | grep -E "OfxGet" || { echo "OFX entry points missing"; exit 1; }
# Ad-hoc signature: required on Apple Silicon for any loadable code.
codesign --force --sign "${CODESIGN_IDENTITY:--}" --timestamp=none "$BUNDLE"
codesign --verify --verbose "$BUNDLE"
echo "OK: $BUNDLE"
