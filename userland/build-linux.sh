#!/bin/sh
# Development build only. The release image is built with NetBSD tools.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
prefix="$root/build/linux/prefix"
mkdir -p "$root/build/linux" "$prefix"
cd "$root/build/linux"
if [ ! -f libXfont-1.5.4.tar.bz2 ]; then
    curl -fL --retry 3 -o libXfont-1.5.4.tar.bz2 https://www.x.org/releases/individual/lib/libXfont-1.5.4.tar.bz2
fi
if [ ! -f "$prefix/lib/pkgconfig/xfont.pc" ]; then
    tar xf libXfont-1.5.4.tar.bz2
    cd libXfont-1.5.4
    ./configure --prefix="$prefix" --disable-shared --disable-devel-docs --disable-fc --disable-freetype
    make -j4
    make install
    cd ..
fi
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
cd "$root/userland/tinyx"
autoreconf -fi
mkdir -p "$root/build/linux/tinyx"
cd "$root/build/linux/tinyx"
"$root/userland/tinyx/configure" --prefix="$prefix" --disable-xvesa --disable-xfbdev --disable-xdmcp --disable-install-setuid CFLAGS='-Os -fcommon -ffunction-sections -fdata-sections' LDFLAGS='-Wl,--gc-sections'
make -j4
make install
