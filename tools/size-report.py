#!/usr/bin/env python3
"""Regenerate the size worksheet from the files actually packed."""
from pathlib import Path
from collections import defaultdict
root=Path(__file__).resolve().parents[1]
release=root/'build/release'
groups=defaultdict(int)
has_terminal=has_dock=False
for line in (release/'root-files.txt').read_text().splitlines():
    size,path=line.split(' ',1)
    name=Path(path).name
    has_terminal=has_terminal or path=='usr/bin/xterm'
    has_dock=has_dock or path=='usr/bin/wbar'
    if name=='Xfbdev':group='TinyX Xfbdev, including built-in fonts'
    elif name.startswith(('perl','libperl')) or path.startswith(('usr/lib/perl5/','usr/share/perl5/','usr/lib/urxvt/')):group='Terminal background extension and Perl runtime'
    elif path.startswith('lib/firmware/') or path.startswith('usr/share/felix/licenses/wifi/') or name in ('wpa_supplicant','wpa_cli','wpa_passphrase','eapol_test','iw') or name.startswith('libnl'):group='Wi-Fi tools, firmware and regulatory data'
    elif name=='flwm':group='flwm with static FLTK core'
    elif name in ('wbar','urxvt','xterm') or name.startswith(('libImlib2','libncurses','libptytty')) or path.startswith(('usr/lib/imlib2/','usr/share/felix/theme/','usr/share/terminfo/')):group='Terminals, wbar and theme assets'
    elif name.startswith('felix-') or path.startswith('root/.wmx/') or path=='init':group='Felix applications, session and menus'
    elif name.startswith(('libapk','libcrypto','libssl')) or path=='sbin/apk' or path.startswith('etc/ssl/'):group='apk, HTTPS libraries and certificates'
    elif name.startswith(('libX','libxcb','libfontenc','libbsd','libmd')) or path.startswith('usr/share/X11/'):group='X client libraries and locale data'
    elif name.startswith(('ld-musl','libstdc++','libgcc')):group='musl and C/C++ runtime libraries'
    elif path=='bin/busybox':group='BusyBox utilities and shell'
    else:group='Remaining base files, licenses and configuration'
    groups[group]+=int(size)
iso=(release/'felix-1.1-x86.iso').stat().st_size
kernel=(release/'vmlinuz').stat().st_size
ram=(release/'initramfs.cpio.xz').stat().st_size
text='# Felix size budget\n\nMeasured from the current Linux x86 release. MB means 1,000,000 bytes. The hard limit applies to the complete bootable ISO.\n\n'
if not (has_terminal and has_dock):text+='**Previous image:** these measurements precede the requested xterm/PMDock rebuild. Their new contribution has not been measured yet. See BUILD_STATUS.md.\n\n'
text+='| ISO component | Bytes | MB |\n|---|---:|---:|\n'
for name,n in [('Linux 6.18.50 tinyconfig-based kernel',kernel),('XZ-compressed RAM root',ram),('BIOS loader, ISO filesystem and hybrid alignment',iso-kernel-ram),('Complete ISO',iso),('Headroom below 29,000,000 bytes',29000000-iso)]:
    text+=f'| {name} | {n:,} | {n/1e6:.3f} |\n'
text+='\nThe following are uncompressed regular-file bytes inside the RAM root; they explain the compressed row above and must not be added to the ISO total.\n\n| Userland group | Bytes | MB |\n|---|---:|---:|\n'
for name,n in sorted(groups.items()):text+=f'| {name} | {n:,} | {n/1e6:.3f} |\n'
text+=f'| Total regular-file content | {sum(groups.values()):,} | {sum(groups.values())/1e6:.3f} |\n'
text+='\nThe kernel uses VESA fbdev and has no DRM, wireless, WLAN, cfg80211 or mac80211. The initial image omits wbar, a compositor, terminal emulator, development tools, documentation packages and kernel modules. TinyX embeds fixed and cursor fonts. FLTK uses Xft with DejaVu Sans and Sans Mono, and excludes OpenGL, Wayland, printing and image-codec libraries; PNG wallpapers use the small Felix root utility and libpng. Only the required core FLTK archive is built.\n\nHTTPS/OpenSSL and the C++ runtime are the largest individual runtime costs. Modern Xorg servers, fonts, browsers, large toolkits and package dependencies can exceed the budget after installation. The initial ISO retains the actual Alpine package database and dependencies rather than removing libraries while claiming packages remain intact.\n\nThe NetBSD base, rump/microvm kernel and pkgin contribute zero bytes to this release: the Linux request superseded that earlier design. The optional persistence disk, build directories and test-only packages are not part of the ISO. `tools/pack-iso.sh` rejects an ISO of 29,000,000 bytes or larger.\n'
if has_terminal:text=text.replace('terminal emulator, ','')
text=text.replace('HTTPS/OpenSSL and the C++ runtime are the largest individual runtime costs.', 'The terminal background extension requires Perl. Felix uses a PNG-only Imlib2 package to avoid unused image-codec dependencies. HTTPS/OpenSSL and the C++ runtime add further space.')
text=text.replace('has no DRM, wireless, WLAN, cfg80211 or mac80211', 'has no DRM. Wireless, cfg80211/mac80211, USB host controllers and selected Intel, Broadcom, Atheros, Ralink, Realtek and MediaTek drivers are enabled')
if has_dock:text=text.replace('omits wbar, a compositor','omits a compositor');text+='\nWbar uses Imlib2, five Tango PNG icons and DejaVu Sans for labels. DejaVu Sans Mono serves the terminal and text editor; bold and italic variants are synthesized by the font renderer. The tinted terminal uses Alpine rxvt-unicode and its background extension, including the required Perl runtime. Xterm remains an opaque fallback. PMDock is not shipped.\n'
(root/'SIZE_BUDGET.md').write_text(text)
print(text)
