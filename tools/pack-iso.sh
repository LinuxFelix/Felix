#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
work=/var/tmp/felix-linux
mode=${1:-all}
case "$mode" in all|rootfs|iso-only) ;; *) echo "usage: $0 [all|rootfs|iso-only]" >&2; exit 2 ;; esac
[ "$(id -u)" = 0 ] || { echo "Run with sudo: chroot and device nodes require root." >&2; exit 1; }
release="$root/build/release"
mkdir -p "$work" "$release"
if [ "$mode" != iso-only ]; then
stage=$(mktemp -d "$work/image.XXXXXX")
mkdir -p "$stage/root"
tar xzf "$root/build/downloads/alpine-minirootfs-3.23.5-x86.tar.gz" -C "$stage/root"
cp /etc/resolv.conf "$stage/root/etc/resolv.conf"
cp /etc/ssl/certs/ca-certificates.crt "$stage/root/etc/ssl/certs/ca-certificates.crt"
mknod -m 666 "$stage/root/dev/null" c 1 3
mknod -m 666 "$stage/root/dev/urandom" c 1 9
chroot "$stage/root" apk add --no-cache libstdc++ libx11 libxext libxrandr libxrender libfontenc zlib ca-certificates-bundle xterm rxvt-unicode imlib2
sh "$root/tools/pack-wifi.sh" "$stage/root"
for binary in Xfbdev flwm felix-apps felix-root felix-about wbar; do cp "$work/alpine/out/$binary" "$stage/root/usr/bin/"; done
cp "$root/userland/bin/felix-session" "$stage/root/usr/bin/"
cp "$root/userland/bin/felix-dock" "$root/userland/bin/felix-terminal" "$stage/root/usr/bin/"
cp "$root/userland/bin/wificonfig" "$root/userland/bin/wifconfig" "$stage/root/usr/bin/"
cp "$root/userland/bin/felix-install" "$root/userland/bin/felix-desktop" "$root/userland/bin/felix-askpass" "$stage/root/usr/bin/"
chmod +x "$stage/root/usr/bin/felix-install" "$stage/root/usr/bin/felix-desktop" "$stage/root/usr/bin/felix-askpass"
touch "$stage/root/etc/felix-live"
cp "$root/userland/bin/startx" "$stage/root/usr/bin/"
chmod +x "$stage/root/usr/bin/startx"
cp -R "$root/userland/etc/skel/." "$stage/root/etc/skel/"
mkdir -p "$stage/root/usr/libexec"
cp "$root/userland/linux/felix-init" "$stage/root/sbin/"
cp "$root/userland/linux/felix-system-start" "$root/userland/linux/felix-installed-console" "$stage/root/usr/libexec/"
chmod +x "$stage/root/sbin/felix-init" "$stage/root/usr/libexec/"*
chmod +x "$stage/root/usr/bin/wificonfig" "$stage/root/usr/bin/wifconfig"
cp "$root/userland/linux/init" "$stage/root/init"
cp -R "$root/userland/etc/skel/." "$stage/root/root/"
mkdir -p "$stage/root/root/Documents" "$stage/root/usr/share/felix"
mkdir -p "$stage/root/usr/share/felix/licenses"
cp "$root/userland/linux/installed-extlinux.cfg" "$root/userland/linux/installed-inittab" "$stage/root/usr/share/felix/"
sha256sum "$release/vmlinuz" > "$stage/root/usr/share/felix/kernel.sha256"
cp -R "$root/userland/share/theme" "$stage/root/usr/share/felix/"
mkdir -p "$stage/root/etc/fonts/conf.d"
cp "$root/userland/etc/fonts/99-felix-dejavu.conf" "$stage/root/etc/fonts/conf.d/"
# Append to Alpine's app defaults, preserving its terminal behavior.
printf '\nXTerm*renderFont: true\nXTerm*faceName: DejaVu Sans Mono\nXTerm*faceSize: 10\n' >> "$stage/root/usr/lib/X11/app-defaults/XTerm"
chroot "$stage/root" fc-cache -f
test "$(chroot "$stage/root" fc-match -f '%{family}' sans-serif)" = 'DejaVu Sans'
test "$(chroot "$stage/root" fc-match -f '%{family}' monospace)" = 'DejaVu Sans Mono'
cp "$root/userland/share/wbar.cfg" "$stage/root/usr/share/felix/"
# Validate the actual packaged icons using the actual packaged decoder.
# This opens no display and keeps a malformed icon from reaching the ISO.
cp "$root/userland/tests/check-dock-images.c" "$work/alpine/work/"
chroot "$work/alpine" cc -Os /work/check-dock-images.c -lImlib2 -o /work/check-dock-images
cp "$work/alpine/work/check-dock-images" "$stage/root/tmp/check-dock-images"
chroot "$stage/root" sh -c '/tmp/check-dock-images /usr/share/felix/theme/*.png'
rm -f "$stage/root/tmp/check-dock-images"
cp "$root/userland/tests/check-theme.c" "$work/alpine/work/"
chroot "$work/alpine" cc -Os /work/check-theme.c -lX11 -o /work/check-theme
cp "$work/alpine/work/check-theme" "$stage/root/usr/share/felix/check-theme"
cp "$root/userland/tests/check-fonts.c" "$work/alpine/work/"
chroot "$work/alpine" sh -c 'cc -Os /work/check-fonts.c $(pkg-config --cflags --libs xft fontconfig) -lX11 -o /work/check-fonts && strip /work/check-fonts'
cp "$work/alpine/work/check-fonts" "$stage/root/usr/share/felix/check-fonts"
cp "$root/userland/wbar/COPYING" "$stage/root/usr/share/felix/licenses/wbar-COPYING"
cp "$root/userland/share/theme/TERMINAL-LICENSE.txt" "$stage/root/usr/share/felix/licenses/rxvt-unicode-COPYING"
cp "$root/userland/tinyx/COPYING" "$stage/root/usr/share/felix/licenses/TinyX-COPYING"
cp "$work/kernel/COPYING" "$stage/root/usr/share/felix/licenses/Linux-COPYING"
cp "$work/kernel/LICENSES/preferred/GPL-2.0" "$stage/root/usr/share/felix/licenses/Linux-GPL-2.0"
cp "$work/alpine/work/fltk-1.4.5/COPYING" "$stage/root/usr/share/felix/licenses/FLTK-COPYING"
cp "$root/userland/flwm/README" "$stage/root/usr/share/felix/licenses/flwm-README"
cp "$release/kernel.config" "$stage/root/usr/share/felix/"
printf 'NAME="Felix"\nID=felix\nID_LIKE=alpine\nVERSION="1.0"\nVERSION_ID="1.0"\nPRETTY_NAME="Felix 1.0"\n' > "$stage/root/etc/os-release"
chmod +x "$stage/root/init" "$stage/root/usr/bin/felix-session" "$stage/root/usr/bin/felix-dock" "$stage/root/usr/bin/felix-terminal" "$stage/root/root/.wmx/"*
chroot "$stage/root" apk info -v > "$release/packages.txt"
# Minimize only this newly-created staging tree, never the source or build root.
find "$stage/root/usr/share/man" "$stage/root/var/cache/apk" -type f -delete 2>/dev/null || :
find "$stage/root" -type f -printf '%s %P\n' | sort -nr > "$release/root-files.txt"
# Preserve Unix permissions, owners, symlinks and device nodes in the cpio.
(cd "$stage/root" && find . -print0) > "$stage/files.unsorted"
LC_ALL=C sort -z "$stage/files.unsorted" > "$stage/files.sorted"
(cd "$stage/root" && cpio --null -o --format=newc --owner=0:0 < "$stage/files.sorted") > "$stage/initramfs.cpio"
xz --check=crc32 --lzma2=dict=8MiB -9 -c "$stage/initramfs.cpio" > "$release/initramfs.cpio.xz.tmp"
mv "$release/initramfs.cpio.xz.tmp" "$release/initramfs.cpio.xz"
(cd "$release" && sha256sum vmlinuz initramfs.cpio.xz > rootfs-build.sha256)
printf '%s\n' "$stage/root" > "$release/staging-root.txt"
echo "Felix rootfs archive: $release/initramfs.cpio.xz"
echo "Expanded rootfs: $stage/root"
if [ "$mode" = rootfs ]; then exit 0; fi
else
    [ -f "$release/staging-root.txt" ] || { echo 'Run make rootfs first.' >&2; exit 1; }
    staged_root=$(cat "$release/staging-root.txt")
    case "$staged_root" in "$work"/image.*/root) ;; *) echo 'Invalid staging-root.txt path.' >&2; exit 1 ;; esac
    [ -d "$staged_root" ] || { echo 'Staging tree is missing; run make rootfs.' >&2; exit 1; }
    (cd "$release" && sha256sum -c rootfs-build.sha256)
    stage=${staged_root%/root}
fi
mkdir -p "$stage/iso/boot/isolinux"
cp "$release/vmlinuz" "$release/initramfs.cpio.xz" "$stage/iso/boot/"
cp /usr/lib/ISOLINUX/isolinux.bin /usr/lib/syslinux/modules/bios/ldlinux.c32 "$stage/iso/boot/isolinux/"
cp /usr/lib/syslinux/modules/bios/vesamenu.c32 /usr/lib/syslinux/modules/bios/libcom32.c32 /usr/lib/syslinux/modules/bios/libutil.c32 "$stage/iso/boot/isolinux/"
cp "$root/userland/linux/isolinux.cfg" "$stage/iso/boot/isolinux/"
cp "$stage/root/usr/share/felix/theme/boot.png" "$stage/iso/boot/isolinux/splash.png"
xorriso -as mkisofs -o "$release/felix-1.0-x86.iso.tmp" -V FELIX -J -R \
    -b boot/isolinux/isolinux.bin -c boot/isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin "$stage/iso"
size=$(stat -c %s "$release/felix-1.0-x86.iso.tmp")
[ "$size" -lt 29000000 ] || { echo "ISO exceeds the 29 MB budget: $size bytes" >&2; exit 1; }
mv "$release/felix-1.0-x86.iso.tmp" "$release/felix-1.0-x86.iso"
(cd "$release" && sha256sum felix-1.0-x86.iso > felix-1.0-x86.iso.sha256)
printf '%s\n' "$stage/root" > "$release/staging-root.txt"
echo "Felix ISO: $size bytes (hard limit 29000000)"
python3 "$root/tools/size-report.py"
printf 'ISO packaged from the staged Felix rootfs with the Syslinux boot menu.\nPackaging and size checks passed; this does not imply boot-test or visual verification. See BUILD_STATUS.md for test results.\n' > "$release/BUILD_STATUS.txt"
