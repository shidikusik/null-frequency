#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

if [[ ! -x build/null-frequency ]]; then
    printf 'Build the game first: make release\n' >&2
    exit 1
fi
if [[ ! -f .deps/raylib/LICENSE ]]; then
    printf 'The dependency license is missing; run ./scripts/build.sh first.\n' >&2
    exit 1
fi

readonly ARCH="$(uname -m)"
readonly NAME="null-frequency-linux-${ARCH}"
readonly STAGE="dist/$NAME"
mkdir -p dist
rm -rf "$STAGE"
mkdir -p "$STAGE/licenses" "$STAGE/docs"
cp build/null-frequency run.sh README.md LICENSE THIRD_PARTY.md "$STAGE/"
cp -R assets "$STAGE/assets"
cp .deps/raylib/LICENSE "$STAGE/licenses/raylib-LICENSE.txt"
cp .deps/raylib/src/external/glfw/LICENSE.md "$STAGE/licenses/GLFW-LICENSE.txt"
if [[ -f docs/screenshot.png ]]; then cp docs/screenshot.png "$STAGE/docs/"; fi
if [[ -f docs/weapon-animation.mp4 ]]; then cp docs/weapon-animation.mp4 "$STAGE/docs/"; fi
tar -czf "dist/$NAME.tar.gz" -C dist "$NAME"
(cd dist && sha256sum "$NAME.tar.gz" > "$NAME.tar.gz.sha256")
printf 'Package: %s/dist/%s.tar.gz\n' "$PWD" "$NAME"
