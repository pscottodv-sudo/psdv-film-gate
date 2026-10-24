#!/bin/bash
# Linux CPU build + automated tests (core numerics and real OFX host).
# Requires: cmake, g++, python3 with numpy, libexpat1-dev.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ${OPENFX_DIR:+-DOPENFX_DIR=$OPENFX_DIR}
cmake --build build -j
[ "${SKIP_CORE:-0}" = 1 ] || python3 tests/run_tests.py build
OFXDIR="$(sed -n 's/^OPENFX_DIR:PATH=//p' build/CMakeCache.txt)"
[ -n "$OFXDIR" ] || OFXDIR=build/_deps/openfx-src
cmake -S tests/ofx_host -B tests/ofx_host/build -DOPENFX_DIR="$(cd "$OFXDIR" && pwd)"
cmake --build tests/ofx_host/build -j
OFX_PLUGIN_PATH="$PWD/build" tests/ofx_host/build/psdv_ofx_host --list | sed -n 1,8p
OFX_PLUGIN_PATH="$PWD/build" tests/ofx_host/build/psdv_ofx_host --size 640 360 --range 0 47 --frames 0,24,47 --out /tmp/psdv_host
