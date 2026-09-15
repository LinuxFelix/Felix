#!/bin/sh
# Debian/WSL host; run as root for the isolated Alpine chroot.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
[ "$(id -u)" = 0 ] || { echo 'Run this build with sudo (chroot and device nodes).' >&2; exit 1; }
python3 "$root/tools/fetch-sources.py"
mkdir -p "$root/build/downloads" "$root/build/linux"
cd "$root/build/downloads"
fetch() { [ -f "$1" ] || curl -fL --retry 3 "$2" -o "$1"; }
fetch linux-6.18.50.tar.xz https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.18.50.tar.xz
fetch alpine-minirootfs-3.23.5-x86.tar.gz https://dl-cdn.alpinelinux.org/alpine/v3.23/releases/x86/alpine-minirootfs-3.23.5-x86.tar.gz
sha256sum -c "$root/tools/sources.sha256"
grep alpine-minirootfs "$root/tools/sources.sha256" > alpine-minirootfs-3.23.5-x86.tar.gz.sha256
grep linux-6.18 "$root/tools/sources.sha256" > linux-6.18.50.tar.xz.sha256
cd "$root/build/linux"
fetch libXfont-1.5.4.tar.bz2 https://www.x.org/releases/individual/lib/libXfont-1.5.4.tar.bz2
cd "$root"
sha256sum -c tools/userland-sources.sha256
sh "$root/tools/prepare-linux.sh"
