#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
work=/var/tmp/felix-linux/alpine
cp "$root/userland/build-fltk.sh" "$work/src/"
chroot "$work" apk add libxft-dev fontconfig
cp "$work/usr/share/fonts/dejavu/DejaVuSansMono.ttf" "$root/userland/share/theme/"
chroot "$work" sh -c 'cd /work/fltk-1.4.5 && make -C src clean'
chroot "$work" sh /src/build-fltk.sh /work/fltk-1.4.5 /opt/felix
sh "$root/tools/rebuild-desktop.sh"
sh "$root/tools/pack-iso.sh"
