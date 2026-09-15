#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
work=/var/tmp/felix-linux
mkdir -p "$work/alpine" "$work/kernel"
cd "$root/build/downloads"
sha256sum -c alpine-minirootfs-3.23.5-x86.tar.gz.sha256
sha256sum -c linux-6.18.50.tar.xz.sha256
if [ ! -f "$work/kernel/Makefile" ]; then tar xf linux-6.18.50.tar.xz -C "$work/kernel" --strip-components=1; fi
if [ ! -f "$work/alpine/bin/sh" ]; then tar xzf alpine-minirootfs-3.23.5-x86.tar.gz -C "$work/alpine"; fi
cp /etc/resolv.conf "$work/alpine/etc/resolv.conf"
cp /etc/ssl/certs/ca-certificates.crt "$work/alpine/etc/ssl/certs/ca-certificates.crt"
for pair in 'null 1 3' 'zero 1 5' 'urandom 1 9' 'random 1 8'; do
    set -- $pair
    [ -e "$work/alpine/dev/$1" ] || mknod -m 666 "$work/alpine/dev/$1" c "$2" "$3"
done
chroot "$work/alpine" /sbin/apk add build-base autoconf automake libtool pkgconf xorgproto xtrans libfontenc-dev libx11-dev libxext-dev zlib-dev freetype-dev util-macros linux-headers imlib2-dev libxrandr-dev libxft-dev font-dejavu tango-icon-theme
mkdir -p "$work/alpine/src"
tar -C "$root/userland" --exclude=.git --exclude=autom4te.cache -cf - . | tar -C "$work/alpine/src" -xf -
cp "$root/service/felix/patches/flwm-config.h" "$work/alpine/src/"
cp "$root/build/linux/libXfont-1.5.4.tar.bz2" "$work/alpine/src/"
