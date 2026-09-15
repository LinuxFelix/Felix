#!/bin/sh
set -eu
src=${1:?source directory}
out=${2:?output directory}
mkdir -p "$out"
${CXX:-c++} -std=c++17 -Os -ffunction-sections -fdata-sections \
    $(fltk-config --cxxflags) "$src/apps/felix-apps.cxx" \
    $(fltk-config --ldflags) -Wl,--gc-sections -o "$out/felix-apps"
strip "$out/felix-apps"
