# Felix 1.1

Stable release for the supported BIOS/x86 configuration, 17 September 2026. ISO: **25,165,824 bytes (24 MiB)**, about 11% smaller than 1.0. SHA-256: `089a1b2090c6904ee0f531ce7bc89ea8975680c3f552b26ba61a859a4f3b75e7`. Final release validation is recorded in `BUILD_STATUS.md`.

Felix 1.1 expands networking and improves the desktop while reducing the live image size. The 1.0 release remains available separately.

## Supported configuration

The release target is BIOS/legacy 32-bit x86, standard VESA VGA, PS/2 input and 512 MB RAM for live use. Automated checks use QEMU/KVM. The installer requires Ethernet and a whole eligible 2 GiB–2 TiB disk, creates an MBR bootloader and ext4 filesystem, and boots a regular disk-backed system with a sudo user.

UEFI-only boot and dual boot are not supported. Physical Ethernet, USB and Wi-Fi adapters have not been validated. Included drivers and optional firmware do not guarantee support for every chipset. Modern software requiring DRM, Wayland or a full Xorg server may not work. Manual network and date/time changes apply to the current session. Live-session changes require persistent storage to survive shutdown.

## Stability fixes

* Ethernet DHCP recovery checks the process command and interface before trusting a PID file. A reused PID no longer prevents DHCP from restarting. Manual IPv4 configuration checks the DHCP process identity before stopping it.
* Firmware completion no longer probes PCI devices on the GUI thread. Reload is an explicit, separate operation for the selected card; operation-status errors restore the controls instead of leaving them disabled.
* USB hardware detection reads bound interface drivers rather than reporting the generic USB device driver as a Wi-Fi driver.
* Opening the Wi-Fi hardware window preserves that selection through administrator authentication.

## Desktop and apps

* Windows slide upward when opened. Closing windows briefly animate an independent titlebar downward; this is not a composited full-window effect. Settings → Desktop → Reduce motion disables both effects.
* Settings → Network lists detected wired adapters and reports cable status and the current IPv4 address. Automatic DHCP and manual IPv4 controls are available.
* Notepad includes F3 Find next, Ctrl+Shift+S Save As, shortcut hints and a document status/size indicator. Settings upgrades references to the built-in 1.0 PPM wallpapers to their PNG replacements.
* Wi-Fi → Hardware identifies PCI wireless-controller vendor/device IDs and USB adapters whose product names identify them as wireless. It reports a bound driver when available. Unrecognized USB devices may not appear; a missing interface alone does not prove a driver is missing.
* Select Intel, Broadcom, Qualcomm/Atheros, Realtek or MediaTek/Ralink to download the associated Alpine firmware package. Downloads are optional and may be large. A vendor selection does not add a kernel driver or guarantee support for every chipset from that vendor. Legacy Broadcom b43 devices may require separately prepared firmware.
* Hardware can rescan devices and reload the selected PCI driver after confirmation. Reloading disconnects that card. Reconnect USB adapters to reprobe them. Installed systems preserve downloaded firmware; live sessions lose it at shutdown.

## Network support

The kernel adds Intel e1000e/igb, Realtek r8169/8139, Broadcom tg3, AMD PCnet, VIA Rhine, virtio-net, VMXNET3, ASIX USB Ethernet, Realtek RTL8152/8153 and CDC Ethernet/NCM. Intel iwlwifi and Broadcom brcm80211/b43 support are built in alongside the previous Wi-Fi drivers. Firmware and device-specific board data may still be required.

The wired-network service discovers interfaces instead of assuming `eth0`, retries DHCP, renews leases and detects newly attached adapters. Selecting manual IPv4 stops DHCP for that interface for the current session. These settings do not yet persist across restart.

## Size and packaging

Only five Tango PNG icons are shipped, now at 24×24. Three full-resolution PNG wallpapers are each about 29–34 KB, with a build limit of 600,000 bytes per wallpaper. No PPM backgrounds are included in the ISO; the wallpaper utility retains PPM compatibility for user files.

The `felix-imlib2` package contains Alpine's verified Imlib2 shared library and PNG loader, with its license and automatically calculated runtime dependencies. Unused image loaders and their codec dependencies are omitted through package management. Its upstream source archive is checksum-verified. It is a PNG-only runtime, not the full upstream image-format collection; software needing other Imlib2 loaders requires replacing it with the full `imlib2` package.

The boot rootfs remains `initramfs.cpio.xz`. Build with `sudo make iso`; output is `build/release/felix-1.1-x86.iso`. See `BUILD_STATUS.md` for measured size, checksum and completed tests. The website gallery currently shows the explicitly labeled 1.0 screenshots.
