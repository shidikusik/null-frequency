#!/usr/bin/env sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
if [ -x ./null-frequency ]; then
    exec ./null-frequency "$@"
fi
if [ -x ./build/null-frequency ]; then
    cd build
    exec ./null-frequency "$@"
fi
printf 'Game executable not found. Run ./scripts/build.sh first.\n' >&2
exit 1
