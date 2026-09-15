#!/bin/sh
# Native ASan regression test; Xvfb is never displayed or captured.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d /var/tmp/felix-menu-test.XXXXXX)
cp "$root/userland/flwm/"*.C "$root/userland/flwm/"*.H "$work/"
cp "$root/service/felix/patches/flwm-config.h" "$work/config.h"
# Debian's test toolkit is FLTK 1.3 (no per-screen scaling API).
# Keep the production menu code intact; Xvfb uses scale 1 throughout.
sed -i 's/sf = Fl::screen_scale(0);/sf = 1.0;/; /for (int i = 0; i < Fl::screen_count(); i++) Fl::screen_scale(i, 1.0);/d' "$work/main.C"
c++ -g -O1 -fsanitize=address -fno-omit-frame-pointer -DTOPSIDE \
    $(fltk-config --cxxflags) "$work/"*.C $(fltk-config --ldflags) -lX11 -o "$work/flwm"
mkdir -p "$work/home/.wmx"
cp -R "$root/userland/etc/skel/.wmx/." "$work/home/.wmx/"
export DISPLAY=:89 HOME="$work/home" ASAN_OPTIONS=detect_leaks=0:abort_on_error=1
Xvfb "$DISPLAY" -screen 0 1024x768x24 -nolisten tcp > "$work/x.log" 2>&1 & xp=$!
wm= app=
trap 'cp "$work/asan.log" "$root/build/menu-asan.log" 2>/dev/null || true; kill ${app:-} ${wm:-} "$xp" 2>/dev/null || true' EXIT
sleep 1
"$work/flwm" > "$work/asan.log" 2>&1 & wm=$!
sleep 1
xmessage -geometry 250x120+100+100 'Felix menu regression' > "$work/app.log" 2>&1 & app=$!
sleep 1
xdotool mousemove 850 650 click 3
sleep 1
if ! kill -0 "$wm" 2>/dev/null; then
    cat "$work/asan.log"
    cp "$work/asan.log" "$root/build/menu-asan.log"
    exit 1
fi
xdotool key Escape
for i in $(seq 1 80); do
    xdotool mousemove 850 650 click 3
    # Exercise frames disappearing while their menu entries still exist.
    if [ "$i" = 20 ]; then
        kill "$app"; wait "$app" || true; app=
        sleep .2
        xdotool key Home Down Up
    fi
    xdotool mousemove 855 655 key Escape
    kill -0 "$wm"
done
cp "$work/asan.log" "$root/build/menu-asan.log"
! grep -q 'ERROR: AddressSanitizer' "$work/asan.log"
echo 'MENU_ASAN_OK: 81 right-click menus and client destruction during popup'
