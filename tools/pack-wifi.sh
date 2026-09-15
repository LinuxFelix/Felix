#!/bin/sh
# Add signed Alpine Wi-Fi packages and only the two required large-family blobs.
set -eu
stage=${1:?image root}
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
builder=/var/tmp/felix-linux/alpine
chroot "$stage" apk add --no-cache wpa_supplicant iw wireless-regdb linux-firmware-ath9k_htc linux-firmware-rtlwifi
mkdir -p "$builder/work/wifi-firmware"
chroot "$builder" apk fetch --output /work/wifi-firmware linux-firmware-other linux-firmware-mediatek
for family in other mediatek; do
    # apk verifies the repository signature before selected members are unpacked.
    for archive in "$builder/work/wifi-firmware/linux-firmware-$family-"*.apk; do
        chroot "$builder" apk verify "/work/wifi-firmware/${archive##*/}"
        case $family in
            other) tar xf "$archive" -C "$stage" lib/firmware/rt2870.bin.zst ;;
            mediatek) tar xf "$archive" -C "$stage" lib/firmware/mediatek/mt7601u.bin.zst lib/firmware/mt7601u.bin.zst ;;
        esac
    done
done
licenses="$root/build/downloads/wifi-licenses-20251125"
mkdir -p "$licenses" "$stage/usr/share/felix/licenses/wifi"
for name in WHENCE LICENCE.open-ath9k-htc-firmware LICENCE.rtlwifi_firmware.txt LICENCE.ralink-firmware.txt LICENCE.ralink_a_mediatek_company_firmware GPL-2; do
    [ -s "$licenses/$name" ] || curl -fL --retry 3 "https://gitlab.com/kernel-firmware/linux-firmware/-/raw/20251125/$name" -o "$licenses/$name"
    cp "$licenses/$name" "$stage/usr/share/felix/licenses/wifi/"
done
printf '%s\n' 'Firmware: signed Alpine 3.23 packages from linux-firmware 20251125.' \
    'ath9k_htc and rtlwifi are installed packages. Only rt2870.bin and mt7601u.bin are extracted from other/mediatek.' \
    'Source and licenses: https://gitlab.com/kernel-firmware/linux-firmware/-/tree/20251125' \
    > "$stage/usr/share/felix/licenses/wifi/SOURCES.txt"
find "$stage/lib/firmware" -type f -exec sha256sum {} \; | sed "s|$stage/||" > "$root/build/release/wifi-firmware.sha256"
cp "$root/build/release/wifi-firmware.sha256" "$stage/usr/share/felix/"
