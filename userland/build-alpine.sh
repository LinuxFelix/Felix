#!/bin/sh
# Run in the isolated x86 Alpine build root, with the source copied to /src.
set -eu
export PATH=/opt/felix/bin:$PATH PKG_CONFIG_PATH=/opt/felix/lib/pkgconfig
export FELIX_CONFIGURE_PLATFORM='--build=i586-alpine-linux-musl --host=i586-alpine-linux-musl'
mkdir -p /work /opt/felix /out
cd /work
if [ ! -f /opt/felix/lib/pkgconfig/xfont.pc ]; then
    tar xf /src/libXfont-1.5.4.tar.bz2
    cd libXfont-1.5.4
    ./configure $FELIX_CONFIGURE_PLATFORM --prefix=/opt/felix --disable-shared --disable-devel-docs --disable-fc --disable-freetype CFLAGS=-Os
    make -j4
    make install
fi
cd /src/tinyx
autoreconf -fi
mkdir -p /work/tinyx
cd /work/tinyx
/src/tinyx/configure $FELIX_CONFIGURE_PLATFORM --prefix=/opt/felix --disable-xvesa --with-int10=stub --disable-xdmcp --disable-install-setuid CFLAGS='-std=gnu99 -Os -fcommon -ffunction-sections -fdata-sections' LDFLAGS='-Wl,--gc-sections'
make -j4
make install
if [ ! -f /opt/felix/lib/libfltk.a ]; then
    cd /work
    tar xf /src/fltk-1.4.5-source.tar.gz
    sh /src/build-fltk.sh /work/fltk-1.4.5 /opt/felix
fi
sh /src/build-flwm.sh /src/flwm /work/flwm /src/flwm-config.h
sh /src/build-apps.sh /src /out
sh /src/build-wbar.sh /src/wbar /out
cc -Os /src/src/felix-root.c -lpng -lX11 -o /out/felix-root
cc -Os /src/src/felix-about.c $(pkg-config --cflags --libs xft) -lX11 -o /out/felix-about
cp /work/flwm/flwm /opt/felix/bin/Xfbdev /opt/felix/bin/Xtinyremote /out/
strip /out/Xfbdev /out/Xtinyremote /out/flwm /out/felix-apps /out/felix-root /out/felix-about /out/wbar
echo ALPINE_BUILD_OK
