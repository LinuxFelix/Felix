#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root/smolBSD"
mkdir -p service/felix-build/etc service/felix-build/postinst
cat >service/felix-build/options.mk <<'EOF'
IMGSIZE=2048
SETS=base.${SETSEXT} etc.${SETSEXT} comp.${SETSEXT} xbase.${SETSEXT} xcomp.${SETSEXT}
ADDPKGS=pkgin pkg_install sqlite3 curl rsync
EOF
cat >service/felix-build/etc/rc <<'EOF'
#!/bin/sh
. /etc/include/basicrc
echo 'Felix development image'
/bin/sh
. /etc/include/shutdown
EOF
cat >service/felix-build/postinst/build.sh <<'EOF'
#!/bin/sh
set -eu
rootdir=$(pwd)
cp /etc/resolv.conf etc/resolv.conf
mkdir -p usr/pkg/etc/pkgin usr/src/felix
echo 'https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/amd64/11.0/All' >usr/pkg/etc/pkgin/repositories.conf
cp -R /etc/openssl etc/
# /mnt is the shared smolBSD tree. Its userland directory is installed below.
(cd /mnt/userland && tar cf - --exclude=.git --exclude=autom4te.cache .) | (cd usr/src/felix && tar xf -)
if chroot . /bin/sh /usr/src/felix/build-netbsd.sh; then
    cp usr/src/felix/felix-runtime.tar /mnt/felix-runtime.tar
    echo 0 >/mnt/felix-build.status
else
    echo 1 >/mnt/felix-build.status
fi
# Let mkimg unmount the development filesystem even if compilation failed.
# Upstream's builder otherwise kills QEMU with dirty guest cache pages.
sync
EOF
# The upstream builder shares its own directory, not its parent.
mkdir -p userland
(cd "$root/userland" && tar cf - --exclude=.git --exclude=autom4te.cache .) | (cd userland && tar xf -)
mkdir -p userland/felix-config
cp "$root/service/felix/patches/flwm-config.h" userland/felix-config/
cp "$root/build/linux/libXfont-1.5.4.tar.bz2" userland/
bmake SERVICE=felix-build BUILDMEM=2048 build
[ "$(cat felix-build.status)" = 0 ] || { echo 'Native compilation failed; development image preserved' >&2; exit 1; }
