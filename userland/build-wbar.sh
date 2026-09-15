#!/bin/sh
# Build the supplied launcher only; omit its separate GTK configuration app.
set -eu
src=${1:?source}; out=${2:?output}
mkdir -p /work/wbar "$out"
cat > /work/wbar/config.h <<'EOF'
#define PACKAGE_NAME "wbar"
#define VERSION "2.3.4-felix"
#define DEFAULT_CONFIGDIR "/etc/wbar.d"
#define DEFAULT_ARGV "--vbar --pos left"
#define PIXMAPDIR "/usr/share/pixmaps"
EOF
c++ -std=gnu++11 -Os -ffunction-sections -fdata-sections -DHAVE_CONFIG_H \
    -I/work/wbar -I"$src/src/utils" $(pkg-config --cflags imlib2 x11 xrandr) \
    "$src"/src/core/*.cc "$src"/src/utils/*.cc \
    $(pkg-config --libs imlib2 x11 xrandr) -Wl,--gc-sections -o "$out/wbar"
strip "$out/wbar"
