#!/bin/sh
# Repackage Alpine's verified Imlib2 runtime as a PNG-only provider.
# abuild records the actual shared-library dependencies of the retained files.
set -eu
apk add --no-cache abuild
apk info -v | grep -qx imlib2-1.12.4-r1
mkdir -p /work/felix-imlib2 /out/imlib-keys
if [ ! -f /root/.abuild/abuild.conf ]; then
    PACKAGER='Felix builder <builder@localhost>' abuild-keygen -a -i -n
fi
cp /etc/apk/keys/*.rsa.pub /out/imlib-keys/
cd /work/felix-imlib2
cat > APKBUILD <<'EOF'
pkgname=felix-imlib2
pkgver=1.12.4
pkgrel=0
pkgdesc="Felix PNG-only runtime from Alpine Imlib2"
url="https://sourceforge.net/projects/enlightenment/"
arch="x86"
license="Imlib2"
options="!check !strip"
source="https://downloads.sourceforge.net/enlightenment/imlib2-$pkgver.tar.gz"
sha512sums="2686d73745316d206ee2e8859a6879a528cfd5ba5ff46a7c7d98a7c9af578a989f4f56a740da2dc52d444a8e927094683736ef2d431decd410b0ac3b33b460a5  imlib2-1.12.4.tar.gz"
build() { :; }
package() {
    mkdir -p "$pkgdir/usr/lib/imlib2/loaders" "$pkgdir/usr/share/imlib2"
    cp -a /usr/lib/libImlib2.so.1 /usr/lib/libImlib2.so.1.12.4 "$pkgdir/usr/lib/"
    cp /usr/lib/imlib2/loaders/png.so "$pkgdir/usr/lib/imlib2/loaders/"
    cp /usr/share/imlib2/rgb.txt "$pkgdir/usr/share/imlib2/"
    mkdir -p "$pkgdir/usr/share/licenses/felix-imlib2"
    cp "$srcdir/imlib2-$pkgver/COPYING" "$pkgdir/usr/share/licenses/felix-imlib2/"
}
EOF
abuild -F -r
cp /root/packages/work/x86/felix-imlib2-*.apk /out/
