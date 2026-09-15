#!/bin/sh
# Runs inside the disposable NetBSD development filesystem, never the host.
set -eu
system=$(/sbin/sysctl -n kern.ostype)
echo "Native build kernel: $system; uname: $(/usr/bin/uname -a)"
case "$system" in NetBSD|smolBSD) ;; *) echo 'NetBSD build required' >&2; exit 1;; esac
PATH=/usr/pkg/bin:/usr/pkg/sbin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R7/bin
export PATH
export MAKE=gmake
src=/usr/src/felix
prefix=/usr/pkg/felix
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:/usr/X11R7/lib/pkgconfig:/usr/pkg/lib/pkgconfig"
export ACLOCAL_PATH=/usr/pkg/share/aclocal:/usr/X11R7/share/aclocal
export FELIX_CONFIGURE_PLATFORM='--build=x86_64-unknown-netbsd --host=x86_64-unknown-netbsd'
# Required by libtool pipelines in a chroot as well as a normal guest boot.
(cd /dev && sh MAKEDEV std fd)
pkgin update
pkgin -y install gmake autoconf automake libtool pkgconf xorgproto xtrans libfontenc xorg-util-macros
cd "$src"
tar xf libXfont-1.5.4.tar.bz2
cd libXfont-1.5.4
./configure $FELIX_CONFIGURE_PLATFORM --prefix="$prefix" --disable-shared --disable-devel-docs --disable-fc --disable-freetype CFLAGS=-Os
gmake -j4
gmake install
cd "$src/tinyx"
autoreconf -fi
./configure $FELIX_CONFIGURE_PLATFORM --prefix=/usr/pkg --disable-xvesa --disable-xfbdev --disable-xdmcp --disable-xdm-auth-1 --disable-install-setuid \
    CFLAGS='-Os -fcommon -ffunction-sections -fdata-sections' LDFLAGS='-Wl,--gc-sections'
gmake -j4
gmake install
cd "$src"
tar xf fltk-1.4.5-source.tar.gz
MAKE=gmake sh build-fltk.sh "$src/fltk-1.4.5" "$prefix"
PATH="$prefix/bin:$PATH" sh build-flwm.sh "$src/flwm" "$src/flwm-build" "$src/felix-config/flwm-config.h"
cp flwm-build/flwm /usr/pkg/bin/flwm
PATH="$prefix/bin:$PATH" sh build-apps.sh "$src" /usr/pkg/bin
cc -Os -I/usr/X11R7/include src/felix-root.c -L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib -lX11 -o /usr/pkg/bin/felix-root
cc -Os -I/usr/X11R7/include src/felix-about.c -L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib -lX11 -o /usr/pkg/bin/felix-about
sh "$src/pack-runtime.sh" "$src/runtime"
tar cf "$src/felix-runtime.tar" -C "$src/runtime" .
echo 'FELIX_NATIVE_BUILD_OK'
