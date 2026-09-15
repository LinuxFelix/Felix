#!/bin/sh
set -eu
src=${1:?usage: build-flwm.sh source-dir output-dir config-header}
out=${2:?}
config=${3:?}
mkdir -p "$out"
cp "$src"/*.C "$src"/*.H "$out/"
cp "$config" "$out/config.h"
cd "$out"
# FLTK's image library is not used by flwm. Do not add --use-images.
${CXX:-c++} -Os -DTOPSIDE -ffunction-sections -fdata-sections \
    $(fltk-config --cxxflags) -o flwm ./*.C \
    $(fltk-config --ldflags) -Wl,--gc-sections
strip flwm
