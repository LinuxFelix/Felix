#!/bin/sh
set -eu
src=${1:?pmdock source directory}
out=${2:?output directory}
mkdir -p "$out"
${CC:-cc} -std=gnu99 -Os -Wall -Wextra $(pkg-config --cflags x11 imlib2) \
    "$src/pmdock.c" -o "$out/pmdock" $(pkg-config --libs x11 imlib2)
strip "$out/pmdock"
