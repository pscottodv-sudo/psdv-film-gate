#!/bin/bash
# Create dist/psdv-film-gate-<version>.pkg, a standard macOS installer that puts
# the bundle in /Library/OFX/Plugins.
# Optional environment variables for a distributable (signed + notarized) build:
#   CODESIGN_IDENTITY  "Developer ID Application: Name (TEAMID)"
#   INSTALLER_IDENTITY "Developer ID Installer: Name (TEAMID)"
#   NOTARY_PROFILE     keychain profile created with: xcrun notarytool store-credentials
# Without them the package is unsigned: fine on your own Mac (right click > Open),
# Gatekeeper will warn on other machines.
set -euo pipefail
cd "$(dirname "$0")/.."
VERSION="$(sed -n 's/^project(psdv_film_gate VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
BUILD_DIR="${BUILD_DIR:-build-macos}"
BUNDLE="$BUILD_DIR/psdv-film-gate.ofx.bundle"
[ -d "$BUNDLE" ] || scripts/build_macos.sh
if [ -n "${CODESIGN_IDENTITY:-}" ]; then
  codesign --force --options runtime --timestamp --sign "$CODESIGN_IDENTITY" "$BUNDLE"
fi
STAGE="$(mktemp -d)"; trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/root/Library/OFX/Plugins" dist
cp -R "$BUNDLE" "$STAGE/root/Library/OFX/Plugins/"
pkgbuild --root "$STAGE/root" --identifier it.psdv.filmgate.pkg --version "$VERSION" \
         --install-location / --scripts packaging/macos/scripts "$STAGE/component.pkg"
sed "s/@VERSION@/$VERSION/g" packaging/macos/distribution.xml > "$STAGE/distribution.xml"
OUT="dist/psdv-film-gate-$VERSION.pkg"
SIGN=(); [ -n "${INSTALLER_IDENTITY:-}" ] && SIGN=(--sign "$INSTALLER_IDENTITY")
productbuild --distribution "$STAGE/distribution.xml" --package-path "$STAGE" \
             --resources packaging/macos/resources ${SIGN[@]+"${SIGN[@]}"} "$OUT"
if [ -n "${NOTARY_PROFILE:-}" ] && [ -n "${INSTALLER_IDENTITY:-}" ]; then
  xcrun notarytool submit "$OUT" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$OUT"
fi
shasum -a 256 "$OUT" | tee "$OUT.sha256"
echo "Installer: $OUT"
