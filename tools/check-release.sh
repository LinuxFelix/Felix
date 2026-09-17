#!/bin/sh
# Inspect release archives without booting or changing the installed system.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root/build/release"
sha256sum -c rootfs-build.sha256
sha256sum -c felix-1.1-x86.iso.sha256
xz -t initramfs.cpio.xz
xz -dc initramfs.cpio.xz | cpio -it > "$root/build/release-1.1-cpio-files.txt"
# Alpine's /etc/os-release is a symlink to this regular file.
xz -dc initramfs.cpio.xz | cpio -i --to-stdout usr/lib/os-release > "$root/build/release-1.1-os-release.txt"
grep -qx 'VERSION_ID="1.1"' "$root/build/release-1.1-os-release.txt"
grep -qx 'init' "$root/build/release-1.1-cpio-files.txt"
grep -qx 'usr/bin/felix-apps' "$root/build/release-1.1-cpio-files.txt"
cmp felix-1.1-x86.iso "$root/web/downloads/felix-1.1-x86.iso"
echo 'RELEASE_ARCHIVE_OK: cpio, version 1.1, checksums and website ISO'
