#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
work=/var/tmp/felix-linux/alpine
cp "$root/userland/apps/felix-apps.cxx" "$work/src/apps/"
cp "$root/userland/apps/wificonfig.h" "$work/src/apps/"
cp "$root/userland/apps/installer.h" "$work/src/apps/"
cp "$root/userland/apps/clock.h" "$work/src/apps/"
cp "$root/userland/apps/aqua.h" "$work/src/apps/"
cp "$root/userland/src/"*.c "$work/src/src/"
cp "$root/userland/tinyx/fb/fbblt.c" "$work/src/tinyx/fb/"
cp "$root/userland/tinyx/miext/shadow/shpacked.c" "$work/src/tinyx/miext/shadow/"
cp "$root/userland/build-wbar.sh" "$work/src/"
cp -R "$root/userland/wbar" "$work/src/"
cp "$root/userland/flwm/"*.C "$work/src/flwm/"
cp "$root/userland/flwm/"*.H "$work/src/flwm/"
cp "$root/service/felix/patches/flwm-config.h" "$work/src/flwm-config.h"
chroot "$work" apk add imlib2-dev libxrandr-dev libxft-dev
chroot "$work" sh -c 'cd /work/tinyx && make -j4 && cp kdrive/fbdev/Xfbdev /out/Xfbdev && strip /out/Xfbdev'
chroot "$work" sh /src/build-wbar.sh /src/wbar /out
chroot "$work" sh -c 'PATH=/opt/felix/bin:$PATH sh /src/build-flwm.sh /src/flwm /work/flwm /src/flwm-config.h && cp /work/flwm/flwm /out/'
chroot "$work" sh -c 'PATH=/opt/felix/bin:$PATH sh /src/build-apps.sh /src /out'
chroot "$work" sh -c 'cc -Os /src/src/felix-root.c -lX11 -o /out/felix-root; cc -Os /src/src/felix-about.c $(pkg-config --cflags --libs xft) -lX11 -o /out/felix-about; strip /out/felix-root /out/felix-about'
