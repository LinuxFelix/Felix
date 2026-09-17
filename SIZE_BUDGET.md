# Felix size budget

Measured from the current Linux x86 release. MB means 1,000,000 bytes. The hard limit applies to the complete bootable ISO.

| ISO component | Bytes | MB |
|---|---:|---:|
| Linux 6.18.50 tinyconfig-based kernel | 3,625,472 | 3.625 |
| XZ-compressed RAM root | 20,119,604 | 20.120 |
| BIOS loader, ISO filesystem and hybrid alignment | 1,420,748 | 1.421 |
| Complete ISO | 25,165,824 | 25.166 |
| Headroom below 29,000,000 bytes | 3,834,176 | 3.834 |

The following are uncompressed regular-file bytes inside the RAM root; they explain the compressed row above and must not be added to the ISO total.

| Userland group | Bytes | MB |
|---|---:|---:|
| BusyBox utilities and shell | 820,500 | 0.821 |
| Felix applications, session and menus | 674,780 | 0.675 |
| Remaining base files, licenses and configuration | 22,939,326 | 22.939 |
| Terminal background extension and Perl runtime | 34,651,162 | 34.651 |
| Terminals, wbar and theme assets | 3,903,636 | 3.904 |
| TinyX Xfbdev, including built-in fonts | 707,056 | 0.707 |
| Wi-Fi tools, firmware and regulatory data | 6,346,320 | 6.346 |
| X client libraries and locale data | 4,547,132 | 4.547 |
| apk, HTTPS libraries and certificates | 5,255,107 | 5.255 |
| flwm with static FLTK core | 427,936 | 0.428 |
| musl and C/C++ runtime libraries | 3,732,308 | 3.732 |
| Total regular-file content | 84,005,263 | 84.005 |

The kernel uses VESA fbdev and has no DRM. Wireless, cfg80211/mac80211, USB host controllers and selected Intel, Broadcom, Atheros, Ralink, Realtek and MediaTek drivers are enabled. The initial image omits a compositor, development tools, documentation packages and kernel modules. TinyX embeds fixed and cursor fonts. FLTK uses Xft with DejaVu Sans and Sans Mono, and excludes OpenGL, Wayland, printing and image-codec libraries; PNG wallpapers use the small Felix root utility and libpng. Only the required core FLTK archive is built.

The terminal background extension requires Perl. Felix uses a PNG-only Imlib2 package to avoid unused image-codec dependencies. HTTPS/OpenSSL and the C++ runtime add further space. Modern Xorg servers, fonts, browsers, large toolkits and package dependencies can exceed the budget after installation. The initial ISO retains the actual Alpine package database and dependencies rather than removing libraries while claiming packages remain intact.

The NetBSD base, rump/microvm kernel and pkgin contribute zero bytes to this release: the Linux request superseded that earlier design. The optional persistence disk, build directories and test-only packages are not part of the ISO. `tools/pack-iso.sh` rejects an ISO of 29,000,000 bytes or larger.

Wbar uses Imlib2, five Tango PNG icons and DejaVu Sans for labels. DejaVu Sans Mono serves the terminal and text editor; bold and italic variants are synthesized by the font renderer. The tinted terminal uses Alpine rxvt-unicode and its background extension, including the required Perl runtime. Xterm remains an opaque fallback. PMDock is not shipped.
