# Build status

## Felix 1.1 — release ready, 2026-09-17

The stable release targets the BIOS/x86 configuration described in `RELEASE-1.1.md`. ISO: `build/release/felix-1.1-x86.iso`, **25,165,824 bytes (24 MiB)**. SHA-256: `089a1b2090c6904ee0f531ce7bc89ea8975680c3f552b26ba61a859a4f3b75e7`. The newc cpio/XZ rootfs is **20,119,604 bytes**. Website and release ISOs are byte-identical.

Release hardening fixes stale DHCP PID handling, validates the DHCP process before manual-network shutdown, removes synchronous PCI reprobes from firmware completion, recovers Wi-Fi operation controls on wait errors, reports USB interface drivers, and preserves the hardware-window choice through sudo. The website now describes 1.1 rather than retaining 1.0 release notes.

Passed on this final ISO: desktop/fonts/terminal/clock/hardware-window startup; opening animations, reduced motion and rapid client destruction; console boot and two startx cycles; 60 verified popup openings, closing a client during a popup and menu logout; all six virtual Ethernet models (e1000, e1000e, RTL8139, PCnet, virtio-net-pci, VMXNET3), including DHCP recovery from a PID file pointing to an unrelated live process. Installation used a new disposable virtual disk, refused a mounted target and passed two disk-only boots, non-root desktop, sudo and persistent documents. Archive checks passed for cpio integrity, embedded version, kernel/rootfs/ISO hashes and the website copy.

Source/backend checks also passed: 81 popup cycles under AddressSanitizer; 1,080 overlapping framebuffer-copy cases; 1,848 shadow-copy cases plus empty-bank/damage handling; real wpa_supplicant control tests for open/WPA2/WPA3 profiles, quoted credentials, save/reload, private permissions and forgetting. These Wi-Fi backend checks do not test radio authentication.

Logs: `build/1.1-release-{build,serial,text,menu,memory,ethernet,installer,blit,shadow,wifi,web,archive}.log`. Physical adapters, firmware downloads/reloads on real hardware and visual appearance remain unverified. No 1.1 screenshots were captured; the gallery is explicitly labeled 1.0. Release status is limited to the documented configuration, not universal hardware compatibility.

## Felix 1.1 preview — historical, 2026-09-17

The preview ISO is built at `build/release/felix-1.1-x86.iso`: **25,165,824 bytes (24 MiB)**, down from 1.0's 28,311,552 bytes. SHA-256: `d9e895e965e85b92c6fac7827de5405b9e47c3f6675e0e640e6b2ceb5718613f`. The final cpio archive is **20,126,888 bytes**. The website download matches the ISO. The 1.0 ISO remains separate, and its kernel/rootfs metadata is preserved in `build/releases/1.0/`.

Added Ethernet drivers and interface discovery, DHCP renewal/hotplug polling, adapter/cable/IP status in Settings, Wi-Fi hardware IDs and optional firmware downloads/reload controls, upward opening and downward titlebar-close animations, reduced motion, Notepad F3/Save As shortcuts and document status, PNG wallpapers, 24px Tango icons and a dependency-tracked PNG-only Imlib2 runtime. Wallpaper files are 29–34 KB each. See `RELEASE-1.1.md` for limitations and details.

Passed on 1.1: desktop/font/terminal/clock and Wi-Fi hardware-window startup; actual upward animation and reduced-motion behavior; rapid destruction during opening; two text/startx cycles; 60 real popup openings and menu logout; 81 native sanitizer menu cycles; and DHCP with e1000, e1000e, RTL8139, PCnet, virtio-net-pci and VMXNET3. Installer verification used only a new disposable QEMU disk and passed mounted-drive refusal, two disk-only boots, non-root desktop, sudo and document persistence. Final cpio version/checksums and website ISO equality passed. The kernel configuration includes the required new drivers and still disables DRM and 64-bit mode.

Logs: `build/1.1-{build,serial,final-serial,text,menu,memory,ethernet,installer,archive}.log`. The full Ethernet and menu runs preceded the final Wi-Fi-dialog/wording refinements; the same kernel/network service and flwm binaries were retained, and the final ISO passed desktop/motion/hardware-window startup and installer checks. Physical Ethernet/USB/Wi-Fi hardware and actual firmware-download/reload behavior have not been tested. This is a preview, not a claim of universal adapter support. No 1.1 VM screenshots were captured; the gallery remains clearly labeled 1.0.

## Felix 1.0 website screenshots

At the user's request, five actual screenshots were captured from the verified
Felix 1.0 ISO: desktop, Notepad, Settings, Applications and Terminal. They were
visually checked and copied unchanged into `web/images/`. The gallery in
`web/screenshots.html` labels them "Felix 1.0 build". Image links and all five
source/copy hashes passed verification. `build/release/screenshots-1.0.json`
records the ISO checksum. Earlier notes about no visual inspection describe
the release testing before this explicit screenshot request.

## Felix 1.0 — release ready

Release date: 2026-09-14. This is the stable 1.0 release for the supported BIOS/x86 configuration described in `RELEASE-1.0.md`. Visual review remains with the user; no graphical VM display was opened or captured.

* ISO: `build/release/felix-1.0-x86.iso`, **28,311,552 bytes**.
* SHA-256: `cbd4ba8fd27def114182c91a976194d4b0ffd6cb0b196be68a562618935de7f9`.
* Rootfs: `build/release/initramfs.cpio.xz`, **23,927,516 bytes**, newc cpio compressed with XZ. Tar generation is removed. The previous tar archive is retained under `build/previous-releases/` only.
* Branding: Felix 1.0 in About, Settings, boot menus, website and embedded `os-release`.

User-facing development notes were replaced with useful task descriptions. Settings reports live versus installed storage correctly and rejects invalid screen-saver/bell values. Installer wording is clearer while preserving destructive-action confirmation; pipe allocation now cleans up correctly on failure. Packaging checks the new ISO's size before replacing the previous release file.

All release checks passed on this exact ISO: serial startup, fonts, terminal and clock; text boot and two startx/stop cycles; 60 real menu popups, client closure during a popup and menu logout; 81 native sanitizer menu cycles; 1,080 overlapping framebuffer cases; 1,848 shadow-copy cases plus empty-bank/damage handling; cpio integrity, embedded version and checksums. The installer completed on a newly created disposable QEMU disk, rejected a mounted target, and passed two disk-only boots with non-root desktop, sudo and persistent document verification. The website ISO is byte-identical to the release.

Logs: `build/release-1.0-{build,serial,text,menu,memory,blit,shadow,installer,archive}.log`. Hardware beyond the documented target and visual appearance have not been verified; no measured speedup is claimed. Older statuses below are historical.

## Glass and dither release

Final ISO regression also passed **60 verified real popup openings**, closing Notepad during a popup, retaining the original flwm process, and using the menu to exit to console (`build/glass-menu-check.log`). All rootfs/kernel checksums passed, and tar successfully listed the complete `initramfs.tar.xz` archive (`build/glass-tar-contents.txt`). No graphical display was opened or captured.

The glass-style FLTK/flwm UI and dithered Ocean, Midnight and Silver wallpapers have been compiled and packaged. Settings offers the wallpaper presets. Notepad adds Ctrl+F, word wrap and per-user Documents paths; the clock opens Settings with child-process cleanup. TinyX uses bulk copies for packed shadow spans and handles empty framebuffer banks; flwm no longer waits for a server round trip after each geometry change. Performance gains have not been benchmarked.

The ISO is **28,311,552 bytes**, SHA-256 `f8d43d6abcb547d11254941579af7fbb418744b44b0792fc8087989ade59eeb2`, and the website download is refreshed. `build/release/initramfs.tar.xz` is a real tar archive of the rootfs (**24,649,284 bytes**). The ISO retains a separate cpio initramfs because Linux requires that format for boot.

Completed checks: Linux shell/Python syntax and Makefile dry run; compilation; image decoding and DejaVu resolution during packaging; 29 MB ISO gate; 1,080 overlapping blit cases; 1,848 shadow-copy cases plus empty-bank/damage handling; native flwm sanitizer test with 81 menu cycles; serial startup/font/clock checks; two text-mode startx/stop cycles and duplicate-session refusal. Logs are `build/glass-build.log`, `build/glass-blit-check.log`, `build/glass-shadow-check.log`, `build/glass-memory-check.log`, `build/glass-serial-check.log` and `build/glass-text-check.log`. Visual review remains with the user. Earlier blocked-build entries below are historical.

## Earlier Makefile work — historical status

`make` / `make iso` now runs preparation, kernel and userland builds, theme generation, rootfs archiving and ISO packaging in order. `make rootfs` produces `initramfs.tar.xz` and `initramfs.cpio.xz` without packaging an ISO. `make iso-only` verifies the existing kernel/archive checksums and repackages the retained staging tree. The previous shell entry point delegates to the Makefile. See README.md for prerequisites, commands and outputs.

Source review confirmed the target dependencies and LF-only shell files. The Windows MSYS GNU Make dry run could not start (`couldn't create signal pipe, Win32 error 5`). No new rootfs or ISO has been built for this change. Full Linux execution remains pending after the earlier automatic approval review spend-cap rejection; the release artifacts below remain unchanged.

## Earlier Aqua redesign — historical status

The latest source replaces the plastic stripe texture with pale Aqua-inspired FLTK controls, white text areas and dark text. Applications now has a clear heading, labels above fields, a separate results area, an instructional empty state, Enter-to-search and a blue Install action. Flwm frames, the desktop clock, terminal colors and generated wallpaper/dock assets use a coordinated lighter blue palette.

**This redesign has not been compiled, boot-tested or included in an ISO.** Automatic approval review rejected the last WSL check because the workspace spend cap was reached. No alternative build route was used. After the cap is increased, run `sh tools/rebuild-aqua.sh` in the prepared Linux build environment, then run the serial, text-boot and menu checks. That script regenerates the artwork, builds the desktop, enforces the 29 MB ISO limit and refreshes the website download.

The ISO currently on disk and in `web/downloads` is the preceding glossy/console release, SHA-256 `e4370f6d25a5c3e66e13f86c55a7dbf6346e2cc6a3ce6c1b2364c3010706db0a` (28,311,552 bytes). It passed normal startup, clock geometry/ticking, two live startx cycles, duplicate-session refusal, and disk installation followed by normal and text-mode boots with sudo startx. The native sanitizer test passed 81 menu cycles on that preceding source.

The older QMP-only menu test did not reliably open a popup. Its replacement now uses direct XTEST and checks that a popup exists, excluding managed window frames. A diagnostic run observed `exit_cb` and X shutdown; the final complete regression rerun was blocked by the spend cap. See `build/glass-menu-input-debug.log`. Do not treat the older QMP results as proof of real popup coverage.

## Previous right-click crash fix

Two flwm menu memory errors were reproduced with AddressSanitizer and fixed:

* The menu allocation removed space for a built-in xterm item even though Felix disables that item. Copying the final entries then overflowed the heap buffer. Counting now uses the same compile-time condition as the copy.
* Closing a client while its popup entry remained visible caused drawing/navigation to read a freed Frame. Menu handlers now confirm that the frame is still in the live list before dereferencing it.

The native sanitizer test passed 81 menu openings, including closing a client during a popup and navigating its stale entry. This test uses Debian FLTK 1.3 with test-only scale-1 compatibility adjustments; the production build remains Alpine x86 with FLTK 1.4.5. Reproduction reports are `build/menu-asan-before.log` and `build/menu-asan-use-after-free.log`; the passing run is `build/menu-memory-test.log`.

The rebuilt ISO remains **28,311,552 bytes**. SHA-256: `00ce14684c2ea46b16a5341d880692e0aab825b404600436e4220aa5b944596d`. Its serial startup, font rendering, terminal and Wi-Fi checks passed (`build/menu-fix-check.log`). An additional 60 automated right-clicks on the actual ISO, including closing Notepad during a popup, retained the original flwm process (`build/menu-iso-test.log`, `build/release/menu-serial.log`). No graphical display was opened or captured.

## Previous DejaVu release

The ISO uses DejaVu Sans for FLTK app controls, flwm titles/menus, About Felix and wbar labels. DejaVu Sans Mono is used for editor text, the tinted terminal and fallback xterm. FLTK now builds with Xft/Xrender; the two complete regular font files are shipped, with synthesized bold/italic styles. Command-line Wi-Fi modes remain independent of X.

The rebuilt ISO is **28,311,552 bytes**, below the 29,000,000-byte cap. SHA-256: `b4ca2aaa645d4b0f4bdcf114247a240d67169f3dbda4e672346c32b477a868dd`.

Serial-only validation passed: Syslinux menu, desktop processes, terminal PTY, dock geometry/root pixmap, Wi-Fi self-test and firmware checks, DejaVu font-file resolution, Lithuanian glyph availability, Xft drawing on TinyX, and Notepad/About process startup. Logs: `build/dejavu-check.log` and `build/release/serial-check.log`. No graphical display was opened or captured; visual review remains with the user. Installer behavior retains the verification from the preceding release below.

## Previous installer release

The ISO now includes **Install Felix** in the desktop menu. It is **28,311,552 bytes** and its SHA-256 sidecar has been verified.

The FLTK wizard provides disk selection, login name/password and confirmation, an ext4 confirmation page, a final selected-drive erase warning, an Ethernet/dependency notice and a progress log. The backend validates device identity and mounted/busy/read-only status, downloads dependencies before erasing, creates an ext4 partition, installs Extlinux and creates a password-protected sudo user. It supports whole-disk BIOS/MBR installation on eligible 2 GiB–2 TiB drives.

The installed kernel mounts the ext4 root directly by PARTUUID with no initramfs root. BusyBox init runs the installed system. The desktop automatically signs in as the created regular user; administrator apps use sudo, and text consoles require login. X uses a private authorization cookie. Root's password is locked. Only ordinary runtime filesystems such as `/tmp` and `/run` use RAM.

Validation of the actual packaged installer passed on a newly created disposable QEMU disk: installation, refusal to erase a mounted target, two boots without the ISO, ext4 mounted read/write at `/`, non-root flwm/wbar, password-authenticated sudo and a document surviving the reboot. Evidence: `build/release/installer-test.log`; test disk location: `build/release/installer-test-disk.txt`. The live ISO also passes the serial-only desktop/terminal/Wi-Fi check in `build/release/serial-check.log`.

No graphical display was opened or captured. Physical SATA/NVMe hardware and wizard interaction have not been tested; appearance and interaction remain for the user's review. The test used an emulated IDE drive. Source and usage details are in README.md.

## Previous charcoal / wbar release

The ISO at `build/release/felix-6.18-x86.iso` now contains the requested visible Syslinux menu, vertical center-left wbar, Tango app icons, a soft charcoal gradient with centered Felix branding, matching flwm/FLTK colors and the tinted terminal. PMDock is absent from the image. Alpine xterm remains installed as a fallback; the default launcher uses Alpine rxvt-unicode with its supported background extension and Perl runtime.

The ISO is **28,311,552 bytes**, below the **29,000,000-byte** limit. The Wi-Fi-enabled 32-bit kernel and WiFConfig remain included.

Nonvisual verification includes native compilation, packaged PNG/font decoding, the Syslinux menu's automatic desktop entry, selection of Recovery through serial, wbar geometry, root-pixmap metadata, terminal PTY execution, desktop process stability and the Wi-Fi utility/firmware checks. Wbar measures **67×323 at +10+222** on the 1024×768 desktop, centered vertically at the left. The terminal accepts its transparency options. Evidence is in `build/release/serial-check.log` and `build/release/recovery-check.log`.

The glass effect uses tinted wallpaper transparency; it does not blur or reveal other application windows. No graphical display was opened or captured. Appearance remains for the user's review. Actual wireless association and persistent-home reboot remain untested. A Fontconfig initialization warning appears in the terminal log but did not prevent startup or PTY execution.

Historical notes below describe earlier images; current release logs and size measurements replace their earlier versions.

## Previous Wi-Fi release

`build/release/felix-6.18-x86.iso` was rebuilt on 2026-09-13 with WiFConfig and wpa_supplicant. It is **17,825,792 bytes**, leaving **11,174,208 bytes** below the 29,000,000-byte limit. The SHA-256 sidecar matches the ISO.

This release includes the FLTK WiFConfig app in the desktop menu and Settings, both `wificonfig` and `wifconfig` commands, scanning, open/WPA2/WPA3 Personal profiles, hidden SSIDs, country selection, saved profiles, forgetting, connection status and DHCP lease renewal. Credentials travel over wpa_supplicant's local socket and are saved in private configuration files. Saved profiles are attempted at boot; persistence across power cycles requires the optional home disk.

Linux 6.18.50 remains 32-bit and based on tinyconfig, with DRM disabled. Wireless, cfg80211/mac80211, USB controllers, ath9k/ath9k_htc, rt2800usb, rtl8xxxu and MT7601U are enabled. Selected firmware, firmware licenses and the regulatory database are packaged. E1000 remains wired Ethernet; a VM with only e1000 has no Wi-Fi radio for WiFConfig to configure.

Completed checks:

* Native 32-bit app and kernel compilation, ISO size gate and checksum verification.
* Real wpa_supplicant control test on a dummy interface in an isolated network namespace: open/WPA2/WPA3 profile parsing, quoted/backslash credentials, lossless save/reload, mode-0600 saved files and forgetting profiles. Evidence: `build/release/wifi-control-test.log`.
* Serial-only ISO boot: TinyX, PMDock and flwm stay running; wired DHCP succeeds; WiFConfig's self-test, wpa_supplicant and iw run; e1000 is not listed as Wi-Fi; packaged firmware hashes match. No desktop restart or absent-label warning. Evidence: `build/release/serial-check.log`.
* The packaged Imlib2 decoder successfully reads the five dock XPM assets.

No graphical display was opened or captured. Physical Wi-Fi scanning, authentication and DHCP on an actual wireless adapter remain untested because no such device was available. The user will review the GUI. Persistence with an attached data disk still awaits its reboot test.

## Previous startup correction

The previous startup fixes are retained in the current ISO.

Included:

* Install Alpine's x86 `xterm` package and its dependencies in the RAM root.
* Build the supplied `userland/pmdock/pmdock.c` using Imlib2 and start it before flwm.
* Add four dock launchers, blue XPM icons and a Terminal menu entry.
* Fix TinyX's byte-aligned full-plane copy path to use overlap-safe `memmove` with the correct row order.
* Correct the previous persistent-home initialization and BusyBox mount syntax.
* Correct the XPM file signature so Imlib2 can load all five dock assets. Validate them with the packaged decoder before creating the ISO.
* Continue to flwm if the dock fails, rather than restarting the entire desktop.
* Probe actual disks before attempting persistence, avoiding the absent-label kernel warning.

Validation completed: Linux compilation and ISO packaging succeeded; the ISO is 14,680,064 bytes, below the 29,000,000-byte limit. Its SHA-256 sidecar matches the generated file. Previously, 1,080 overlapping rectangle cases passed with the local C compiler using the actual changed copy path.

After the user's black-screen report, a serial-only diagnostic identified the dock image loading error that caused the restart loop. The corrected image passes `tools/check-serial.py`: TinyX, PMDock and flwm remain running, no session restart is logged, and the missing-label warning is absent. Evidence: `build/release/serial-check.log`. No graphical display was opened or captured. The user will review appearance and interaction; persistence with an attached disk still awaits its reboot test.

The earlier spend-cap block did not recur on the user's subsequent explicit build request. The build completed through the normal Linux build tools.

Older screenshots and GUI test logs in `build/release/` apply to the previous desktop image. Only the new serial check and packaged-icon decoder check cover this startup correction.
