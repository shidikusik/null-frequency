#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

readonly RAYLIB_VERSION=5.5
readonly RAYLIB_SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
readonly RAYLIB_URL="https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/${RAYLIB_VERSION}"
readonly DEPS_DIR="$PWD/.deps"
readonly RAYLIB_DIR="$DEPS_DIR/raylib"
readonly ARCHIVE="$DEPS_DIR/raylib-${RAYLIB_VERSION}.tar.gz"

if [[ "$(uname -s)" != Linux ]]; then
    printf 'This helper targets Linux. Use CMake on Windows or other platforms.\n' >&2
    exit 1
fi
for tool in "${CXX:-g++}" make curl tar sha256sum; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Required build tool is missing: %s\n' "$tool" >&2
        exit 1
    fi
done

if [[ ! -f "$RAYLIB_DIR/src/Makefile" ]]; then
    mkdir -p "$DEPS_DIR"
    if [[ ! -f "$ARCHIVE" ]]; then
        printf 'Downloading raylib %s...\n' "$RAYLIB_VERSION"
        curl --fail --location --retry 3 --connect-timeout 20 --max-time 600 \
            "$RAYLIB_URL" -o "$ARCHIVE.part"
        mv "$ARCHIVE.part" "$ARCHIVE"
    fi
    printf '%s  %s\n' "$RAYLIB_SHA256" "$ARCHIVE" | sha256sum --check -
    staging="$DEPS_DIR/raylib-extract.$$"
    trap 'rm -rf "$staging"' EXIT
    mkdir -p "$staging"
    tar -xzf "$ARCHIVE" --strip-components=1 -C "$staging"
    mv "$staging" "$RAYLIB_DIR"
    trap - EXIT
fi

jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '2')}"
if [[ -z "${JOBS:-}" && "$jobs" -gt 8 ]]; then jobs=8; fi
make -C "$RAYLIB_DIR/src" -j"$jobs" \
    PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE \
    GRAPHICS=GRAPHICS_API_OPENGL_33 \
    GLFW_LINUX_ENABLE_X11=TRUE GLFW_LINUX_ENABLE_WAYLAND=FALSE

if [[ "${1:-}" == --deps-only ]]; then exit 0; fi
if [[ "$#" -gt 0 ]]; then
    printf 'Usage: %s [--deps-only]\n' "$0" >&2
    exit 1
fi

mkdir -p build
flags=(-std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wl,--as-needed -Isrc -isystem "$RAYLIB_DIR/src")
if [[ "${STATIC_RUNTIME:-0}" == 1 ]]; then
    flags+=(-static-libstdc++ -static-libgcc)
fi
"${CXX:-g++}" "${flags[@]}" src/main.cpp src/core.cpp src/audio.cpp \
    "$RAYLIB_DIR/src/libraylib.a" -lGL -lm -lpthread -ldl -lrt -lX11 \
    -o build/null-frequency
rm -rf build/assets
cp -R assets build/assets
printf '\nBuilt %s/build/null-frequency\nRun: ./run.sh\n' "$PWD"
