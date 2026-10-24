#!/bin/bash
# Remove psdv-film gate emulation from this Mac.
set -euo pipefail
sudo rm -rf "/Library/OFX/Plugins/psdv-film-gate.ofx.bundle"
sudo pkgutil --forget it.psdv.filmgate.pkg 2>/dev/null || true
echo "Removed. Restart DaVinci Resolve."
