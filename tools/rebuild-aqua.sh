#!/bin/sh
# Run in the prepared Debian/Alpine build environment.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
python3 "$root/tools/make-theme.py"
sh "$root/tools/rebuild-desktop.sh"
sh "$root/tools/pack-iso.sh"
# Update the static site's download only after a successful size-gated build.
python3 "$root/tools/build-web.py"
