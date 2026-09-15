# Felix 1.0

Felix targets a **32-bit Linux 6.18.50 kernel**, TinyX **Xfbdev**, **flwm**, and native **FLTK 1.4.5** applications, including **WiFConfig**. Its blue glass-style desktop has a vertical **wbar** centered on the left, recognizable Tango app icons and a tinted **rxvt-unicode** terminal. PMDock is not shipped. The kernel starts from `make ARCH=x86 tinyconfig`; DRM remains disabled. Wireless, cfg80211/mac80211 and selected Wi-Fi drivers are enabled.

**Build status:** the current ISO and its SHA-256 sidecar are under `build/release/`. See `BUILD_STATUS.md` for completed checks and `SIZE_BUDGET.md` for measured size. Visual review remains with the user.

This is the Linux implementation requested after the original NetBSD prompt. `smolBSD/`, the NetBSD development scripts and the experimental wscons source are retained as earlier work; they are not used by the ISO build. NetBSD pkgin binaries cannot run here. Applications uses **Alpine 3.23's x86 main/community repositories**, which match the image's musl-based 32-bit userland.

## Run the ISO

The JavaScript-free project website is in `web/`. Open `web/index.html` locally or serve the directory with a static web server. It includes Downloads, About and Screenshots, local DejaVu fonts, archived screenshots and a copy of the release ISO with its checksum. Run `python tools/build-web.py` after updating a release to refresh that copy.

The output is `build/release/felix-1.0-x86.iso`.

The visible Syslinux/ISOLINUX menu offers **Start Felix desktop**, **Start Felix with boot messages**, **Recovery shell (text mode)** and **Felix text mode (startx)**. The desktop entry starts automatically after eight seconds and selects the 1024×768 VESA mode. Arrow keys select an entry; Enter boots it; Tab edits its boot arguments.

Choose **Felix text mode (startx)** to boot without X while retaining the framebuffer. Type `startx` to launch the desktop. The installed boot menu has the equivalent **Felix text login (startx)** entry; log in, then run `startx` (sudo requests your password). **Exit to console** in the flwm menu ends the graphical session; run `startx` to return. Recovery uses plain VGA and is separate from the framebuffer-capable text mode.

Flwm now paints reflective, two-tone titlebars and beveled glass-style controls, with warm red close buttons and distinct active/inactive colors. FLTK apps use pale glass-style controls, white content areas and dark text. These are opaque drawing effects without a compositor. A small borderless digital clock at the desktop's top right shows 24-hour time with seconds, the date and system timezone (UTC by default). It runs as the desktop user and stops with the session.

In PowerShell:

```powershell
cd D:\Laurynas\Felix
.\run-felix.ps1
```

Or use a BIOS/legacy VM with a standard VGA adapter, PS/2 input, an Intel e1000 wired NIC, one CPU and 512 MB RAM:

```sh
qemu-system-i386 -m 512 -cdrom build/release/felix-1.0-x86.iso \
  -boot d -vga std -nic user,model=e1000 -serial stdio
```

The ISO also has a hybrid MBR. UEFI-only boot, USB input, sound, accelerated graphics and arbitrary physical hardware are outside this minimal kernel's supported target. USB host controllers support the included Wi-Fi and storage drivers. QEMU's normal VGA window displays the desktop; a VNC client is not required. Optional QEMU VNC should bind to loopback.

Right-click the desktop to open the flwm menu:

* **Notepad** creates, opens and saves text documents. Ctrl+N, Ctrl+O and Ctrl+S are supported. It prompts before discarding edits and writes through a temporary file before replacing an existing document.
* **Settings** changes mouse acceleration, the background color, a P6 PPM wallpaper, screen saver, keyboard repeat and bell volume. It also applies wired IPv4/DNS settings, sets UTC date/time, reports available space and shuts down the VM. Desktop settings are stored in `~/.felix/settings`; network and clock changes apply to the current session.
* **WiFi Configuration** opens WiFConfig, also available from Settings → Network or the commands `wificonfig` and `wifconfig`. Enable an adapter, scan, choose an SSID, enter its password, and select **Save & connect**. It uses wpa_supplicant's local control socket for open, WPA/WPA2 Personal and WPA3 Personal networks. It supports hidden SSIDs, a country code, saved profiles, disconnect and forgetting networks. Enterprise/802.1X and WEP are not configured by this app. DHCP runs after authentication and renews the lease in the background.
* **Applications** refreshes repository indexes, searches packages, previews dependencies, installs a named package and lists installed packages. Operations show their output in the window and run without blocking the UI. Installing extra software can grow the running system beyond the ISO's initial size budget. Package compatibility with this minimal X server and kernel varies; modern applications requiring DRM, Wayland or a full Xorg stack are not guaranteed to run.
* **Run command** launches an installed graphical program by command name. Its output is written to `/tmp/felix-launch.log`. Terminal programs can run in the included `xterm` or the local serial shell.
* **Terminal** launches Alpine rxvt-unicode with a blue-tinted transparent background, pale text, a pale aqua cursor and a left scrollbar. Its background extension samples the shared desktop wallpaper and updates when the window moves. This is wallpaper transparency, not compositor blur or transparency through other windows. DejaVu Sans Mono is used for terminal and editor text; DejaVu Sans is the default for app controls, flwm titles, menus and dock labels. FLTK uses Xft for scalable, antialiased text. The ordinary `xterm` command remains available as an opaque fallback.

Wbar is built from the supplied `userland/wbar/` source. Five launchers open Terminal, Notepad, Settings, Applications and WiFConfig. `userland/share/wbar.cfg` defines its vertical, center-left placement, subtle zoom, translucent plate and labels. The app icons are unmodified Tango PNG assets; the reflective dock plate and dithered wallpapers with centered Felix branding are generated by `tools/make-theme.py`. Font and icon notices are included with the assets in `userland/share/theme/`. Windows can cover the dock; the same apps remain available from the desktop menu. An About window no longer opens automatically.

Settings → Desktop includes Ocean glass, Midnight glass and Silver mist wallpaper presets; choose one and select Save and apply. Notepad supports Ctrl+F search and optional word wrap, and starts file dialogs in the current user's Documents folder. Clicking the clock opens Settings.

The live root is writable RAM. Save documents under `/root/Documents`. Without a persistence disk, documents, settings and installed packages disappear when the VM powers off.

## Optional persistent documents and desktop settings

For a normal disk installation that also preserves installed packages, use **Install Felix** from the live desktop menu. The optional data image below is only for live sessions.

## Install Felix to a hard drive or SSD

Boot the live ISO in BIOS/legacy mode and connect Ethernet. Open **Install Felix** from the desktop menu. Select a drive, create a login name and password, accept the ext4 layout, then review the final drive summary and check the erase confirmation before selecting **Proceed and install**.

The installer uses the entire selected disk and erases all its partitions. It supports eligible unmounted, writable drives from 2 GiB to 2 TiB and excludes mounted, busy and read-only devices. It is a BIOS/MBR installer; it does not configure dual boot or UEFI boot. The kernel includes IDE/SATA AHCI and NVMe support. Target identity is checked again before destructive operations, and the matching ISO must stay attached until installation finishes.

Before erasing, it requires an Ethernet link and downloads `sudo`, `e2fsprogs` (including `mkfs.ext4`), `sfdisk`, `syslinux`, `blkid` and `xauth` from the configured Alpine repository. A download failure stops the operation before formatting. Installation progress and errors appear in the wizard.

The installed system boots its kernel from disk through Extlinux and mounts the ext4 partition directly using `root=PARTUUID=...`. There is **no initramfs root or RAM-only installation**. `/`, `/usr`, `/etc`, `/var` and `/home` remain on the disk; only `/run`, `/tmp` and device/runtime filesystems use memory. Documents, settings, packages and system changes persist normally.

The created account is a regular user in `wheel`, with password-protected sudo access. The desktop signs into that account automatically. Text-console login requires its password; the root password is locked. X runs as root for framebuffer access, while flwm, wbar and ordinary apps run as the created user using a private X authorization cookie. Settings, Applications and WiFConfig request the user's sudo password when administrator access is needed. After installation, remove the ISO and reboot.

`tools/test-installer.py` uses only a newly created disposable QEMU disk: it installs the actual packaged backend, boots twice with no ISO attached, checks the ext4 root and non-root desktop, authenticates sudo, verifies a persistent document and checks mounted-disk refusal. It opens no graphical display. The installation log is `build/release/installer-test.log`; the virtual disk location is recorded in `build/release/installer-test-disk.txt`.

## Optional live-session data image

Create a new image file on the Linux build host (this command refuses to replace an existing file):

```sh
if test ! -e build/felix-data.img; then
  truncate -s 128M build/felix-data.img
  mkfs.ext4 -F -L FELIXDATA build/felix-data.img
fi
```

Attach it as an IDE disk. Felix mounts the filesystem labelled `FELIXDATA` and keeps `/root` on it. This preserves documents, desktop settings and saved Wi-Fi profiles, but does not persist system-wide package installations or manual wired network settings.

```powershell
.\run-felix.ps1 -DataDisk .\build\felix-data.img
```

## Build on Debian / Debian WSL

The Git repository contains Felix sources and patches, not vendored TinyX,
flwm, wbar, FLTK archives, fonts or icon binaries. `make sources` downloads
pinned upstream revisions and applies the patches in `patches/`; normal
`make rootfs` and `make iso` do this automatically. Alpine supplies fonts,
icons and other dependencies during preparation. Existing vendor trees are
checked and preserved rather than reset. Keep the attribution notices when
redistributing a built ISO.

The build requires network access and root for the isolated Alpine chroot. It uses `/var/tmp/felix-linux` for native Linux filesystem performance; downloads and deliverables stay under `build/`. Allow about 3 GB of working space. Build packages are not included in the ISO.

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends git build-essential autoconf automake \
  libtool pkg-config flex bison bc libelf-dev libssl-dev curl ca-certificates xz-utils \
  cpio xorriso isolinux syslinux-common qemu-system-x86 python3 python3-pil
cd /mnt/d/Laurynas/Felix
sudo make rootfs   # build kernel, userland and root filesystem archives
sudo make iso      # build everything and create the bootable ISO
make test
```

Plain `sudo make` also builds the ISO. You only need one build command: `make iso` already includes the rootfs stage. `sudo make iso-only` repackages the last completed rootfs without recompiling, after checking its kernel/archive checksums. It requires the retained staging directory. Use `make help` to list the targets; `sudo make kernel` and `sudo make userland` build individual components. `JOBS=4` changes kernel build parallelism. The stages run sequentially because they share one Alpine chroot.

Build outputs are under `build/release/`:

* `initramfs.cpio.xz`: XZ-compressed newc cpio root filesystem used by the live ISO, preserving Unix permissions, ownership, symlinks and device nodes.
* `vmlinuz` and `kernel.config`: kernel and effective configuration.
* `felix-1.0-x86.iso` and its `.sha256` sidecar: bootable Syslinux ISO, produced by `make iso`.
* `rootfs-build.sha256`: checksums of the kernel and cpio root filesystem.
* `staging-root.txt`: path to the expanded rootfs, retained under `/var/tmp/felix-linux/image.XXXXXX/root` on the Linux filesystem to preserve Unix metadata.

Felix 1.0 uses cpio only; no tar rootfs is generated. The XZ compression keeps the boot image small. See `RELEASE-1.0.md` for the supported configuration and release checks.

`make rootfs` stops before ISO packaging and leaves any existing ISO unchanged. The archive is a live rootfs, not a replacement for the disk installer, which configures the installed root, account and bootloader. Build targets require a Debian/Linux environment; run them inside WSL rather than native Windows Make. `tools/build-iso.sh` remains available and delegates to `make iso`. `make website` updates the static site's copy of an existing release.

From PowerShell after installing the dependencies in Debian:

```powershell
wsl -d Debian -u root -- make -C /mnt/d/Laurynas/Felix iso
wsl -d Debian -- python3 /mnt/d/Laurynas/Felix/tools/test-iso.py
```

The kernel and Alpine root archives are version-pinned and SHA-256 checked. Alpine packages follow the pinned 3.23 branch's updates; `build/release/packages.txt` records the versions actually included. The final ISO has a SHA-256 sidecar and a strict **29,000,000-byte** build gate. See `SIZE_BUDGET.md` for the measured breakdown.

All Felix-specific applications and boot/session scripts live in `userland/`. The build uses the supplied TinyX/flwm sources and FLTK archive. The effective kernel configuration is saved as `build/release/kernel.config` and included inside the image under `/usr/share/felix/`.

## Wi-Fi hardware and profiles

**Intel e1000 is wired Ethernet.** A VM using e1000 can access the internet through the host's Wi-Fi connection, but it cannot scan or configure the host's wireless radio. WiFConfig detects actual wireless interfaces and reports when none is present. Use a supported physical adapter, or pass one through a VM that supports USB/PCI passthrough, then click Refresh.

The kernel includes ath9k PCI, ath9k_htc USB (including AR9271), rt2800usb (including RT5370), rtl8xxxu (including RTL8188EU) and MT7601U drivers. Firmware for those families and the wireless regulatory database are included; this is not universal Wi-Fi hardware support. Intel iwlwifi adapters are not included. DRM remains disabled.

WiFConfig stores credentials in `~/.felix/wifi/<interface>.conf`, inside a private directory with mode-0600 files. These are local configuration files, not encrypted secrets. Boot automatically attempts saved profiles for detected Wi-Fi interfaces. Without the optional persistent home disk, profiles disappear at shutdown. Status and startup/DHCP logs are under `/run/felix-wifi/`.

`sudo unshare --net python3 tools/test-wifi-control.py` tests the compiled app's backend against the real Alpine wpa_supplicant on an isolated dummy interface: profile configuration, quoted credentials, private file permissions, saving, reloading and forgetting. It opens no display and does not test radio association. `sudo python3 tools/check-serial.py` checks the packaged ISO's boot, desktop processes, wireless tools and firmware without opening or capturing the graphical display. A physical Wi-Fi connection still requires a supported adapter for testing.

## Theme mechanisms

`service/felix/patches/flwm-config.h` sets slate active titlebars (`ACTIVE_COLOR=0x657384`); `build-flwm.sh` compiles with `TOPSIDE`. The session sets inactive titlebars through the actual `FLWM_TITLEBAR_COLOR=48:50:5B` variable. FLTK apps and flwm menus use charcoal surfaces and pale text. `felix-root` publishes a retained `_XROOTPMAP_ID` so wbar and the terminal can share the wallpaper. Replacing it frees the previous retained pixmap. Settings can still apply a solid color or another P6 PPM wallpaper; the **Default desktop** menu item restores Felix's gradient for the current session. Existing saved wallpaper preferences are preserved.

## Verification

`tools/test-iso.py` boots the actual ISO under KVM, checks the 32-bit kernel, TinyX readiness and repository access, and captures `build/release/felix-linux-desktop.png`. It requires `/dev/kvm`; the ordinary run command also works with QEMU's software emulation. Boot evidence is recorded in `build/release/boot-test.log`.

`python3 tools/test-blit.py` compiles and tests TinyX's actual byte-copy path against immutable reference images for 1,080 overlapping rectangles. The earlier copy fix is retained. `tools/test-dock.py` targets the previous PMDock desktop and is historical; it is not the wbar release check. The current nonvisual check is `tools/check-serial.py`, which verifies desktop processes, wireless tools, root-pixmap metadata, center-left dock geometry and terminal PTY execution. The packaged image decoder also validates every theme PNG and wbar's font before ISO creation. Appearance and interaction are left to the user for visual review.

In the serial shell, check `uname -m`, `ps`, `cat /tmp/tinyx.log`, `cat /tmp/network.log`, and `DISPLAY=:0 felix-root '#AEE1FF'`. Verify the menu, open the apps, save and reopen a text file, and use Settings to change the background. On the host, run `sha256sum -c felix-1.0-x86.iso.sha256` from `build/release`, or compare the file hash with its sidecar.

The live ISO runs its desktop as root and has a serial debugging shell. The installed system runs the desktop as the created user, uses sudo for administration and requires a login on the serial console. X11 TCP listening is disabled in both modes.
