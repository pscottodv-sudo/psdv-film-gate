#!/bin/bash
# Copy the freshly built bundle into the system OFX folder (asks for the admin password).
set -euo pipefail
cd "$(dirname "$0")/.."
SRC="${BUILD_DIR:-build-macos}/psdv-film-gate.ofx.bundle"
DEST="/Library/OFX/Plugins"
[ -d "$SRC" ] || { echo "Bundle not found: run scripts/build_macos.sh first"; exit 1; }
sudo mkdir -p "$DEST"
sudo rm -rf "$DEST/psdv-film-gate.ofx.bundle"
sudo cp -R "$SRC" "$DEST/"
sudo xattr -dr com.apple.quarantine "$DEST/psdv-film-gate.ofx.bundle" 2>/dev/null || true
echo "Installed in $DEST/psdv-film-gate.ofx.bundle - restart DaVinci Resolve."
