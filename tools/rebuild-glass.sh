#!/bin/sh
# Refresh the complete desktop, cpio rootfs and ISO from current sources.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
exec sh "$root/tools/rebuild-aqua.sh"
