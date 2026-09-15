#!/bin/sh
# Explicit runtime closure. Compiler, pkgin, Xorg, fonts, locale and docs stay
# in the disposable build image. Licenses are copied separately below.
set -eu
dest=${1:?destination required}
[ ! -e "$dest" ] || { echo "Destination already exists: $dest" >&2; exit 1; }
mkdir -p "$dest"
copy_binary() {
    binary=$1
    [ -f "$dest$binary" ] && return
    mkdir -p "$dest$(dirname "$binary")"
    cp -L "$binary" "$dest$binary"
    # NetBSD ldd emits absolute resolved dependencies after =>.
    ldd "$binary" | awk '/=> \// {print $3}' | while read -r lib; do
        mkdir -p "$dest$(dirname "$lib")"
        cp -L "$lib" "$dest$lib"
    done
}
for binary in /bin/sh /sbin/init /sbin/mount /sbin/mount_ffs /sbin/mount_tmpfs \
    /sbin/ifconfig /sbin/route /sbin/poweroff /bin/hostname /bin/mkdir \
    /bin/chmod /bin/cat /bin/sleep /bin/ls /bin/ps /usr/bin/wc \
    /usr/pkg/bin/Xtinyremote /usr/pkg/bin/flwm /usr/pkg/bin/felix-root /usr/pkg/bin/felix-about; do
    copy_binary "$binary"
done
copy_binary /libexec/ld.elf_so
find "$dest" -type f -exec strip --strip-unneeded {} \;
mkdir -p "$dest/etc" "$dest/root/.wmx" "$dest/tmp" "$dest/dev" "$dest/var/log"
chmod 1777 "$dest/tmp"
cp /usr/src/felix/bin/felix-session "$dest/usr/pkg/bin/"
cp /usr/src/felix/etc/skel/.wmx/* "$dest/root/.wmx/"
chmod +x "$dest/usr/pkg/bin/felix-session" "$dest/root/.wmx/"*
cp /usr/src/felix/etc/rc "$dest/etc/rc"
cp /etc/master.passwd /etc/passwd /etc/pwd.db /etc/spwd.db /etc/group "$dest/etc/"
cp /dev/MAKEDEV /dev/MAKEDEV.local "$dest/dev/"
mkdir -p "$dest/etc/X11" "$dest/usr/share/licenses/felix"
cp /usr/src/felix/etc/X11/tinyx.conf "$dest/etc/X11/"
cp /usr/src/felix/tinyx/COPYING "$dest/usr/share/licenses/felix/tinyx.txt"
cp /usr/src/felix/fltk-1.4.5/COPYING "$dest/usr/share/licenses/felix/fltk.txt"
printf '%s\n' 'Felix uses NetBSD, TinyX, FLTK and flwm; see bundled sources.' >"$dest/usr/share/licenses/felix/NOTICE"
echo '/dev/ld0a / ffs rw 1 1' >"$dest/etc/fstab"
