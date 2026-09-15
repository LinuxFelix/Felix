#!/bin/sh
set -eu
src=${1:?usage: build-fltk.sh extracted-source prefix}
prefix=${2:?}
cd "$src"
./configure ${FELIX_CONFIGURE_PLATFORM:-} --prefix="$prefix" --disable-test --disable-forms --disable-gl \
    --disable-wayland --disable-print --disable-svg --disable-threads \
    --disable-xcursor --disable-xfixes --enable-xft --disable-xinerama \
    --enable-xrender --disable-fluid CFLAGS=-Os CXXFLAGS=-Os
# Only the core toolkit is needed; do not build image codecs or examples.
${MAKE:-make} -C src -j4 ../lib/libfltk.a
mkdir -p "$prefix/lib" "$prefix/include" "$prefix/bin"
cp lib/libfltk.a "$prefix/lib/"
cp -R FL "$prefix/include/"
cp fltk-config "$prefix/bin/"
chmod +x "$prefix/bin/fltk-config"
