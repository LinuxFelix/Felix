# Felix 1.0

Stable release, 14 September 2026. The release checks below passed for the documented configuration. ISO size: 28,311,552 bytes; SHA-256: `cbd4ba8fd27def114182c91a976194d4b0ffd6cb0b196be68a562618935de7f9`.

Felix 1.0 is the first release of the small 32-bit Felix desktop. It includes the glass-style desktop, dithered wallpaper choices, DejaVu fonts, Notepad, Settings, Applications, Wi-Fi configuration and the disk installer.

## Release files

* `build/release/felix-1.0-x86.iso` and its SHA-256 sidecar.
* `build/release/initramfs.cpio.xz`: the compressed newc cpio live root filesystem. No tar rootfs is generated.
* `build/release/vmlinuz`, `kernel.config`, `packages.txt` and `rootfs-build.sha256` record the kernel, configuration, installed packages and archive checksums.

Run `sudo make iso` to build the ISO or `sudo make rootfs` to build the cpio archive. The filename version describes Felix; the kernel remains Linux 6.18.50. Repository packages are drawn from Alpine 3.23 x86, and exact package versions are recorded with each build.

## Improvements

App messages describe tasks and actionable errors. Settings distinguishes live sessions from installed systems, validates screen-saver and bell values, and identifies Felix 1.0. Wi-Fi configuration no longer names a particular virtual Ethernet adapter when no wireless device is present. About and both boot menus use release branding.

The installer retains explicit disk-erasure confirmation and the Ethernet requirement, with clearer explanations. Its pipe-allocation failure path now closes already-open descriptors. ISO packaging validates size before replacing the previous ISO.

This release retains the flwm popup memory fixes, asynchronous window geometry submission, TinyX bulk shadow copies, overlapping-copy fixes, private installed-session X authentication, password-protected sudo and disk-backed installation.

## Supported configuration

The release target is BIOS/legacy 32-bit x86 with standard VESA VGA, PS/2 input and 512 MB RAM for live use. Automated checks use QEMU/KVM. The installer uses a whole eligible 2 GiB–2 TiB disk with an MBR bootloader and ext4; it requires Ethernet for installation dependencies. Documents and installed applications persist on an installed system. Live sessions require persistent storage for files to survive shutdown.

UEFI-only boot and dual boot are not supported. Physical Wi-Fi adapters, arbitrary hardware and every application in the Alpine repository have not been validated. Date/time adjustments in Settings apply to the current session. These limits define the 1.0 support scope; the release designation does not imply universal hardware compatibility or an absence of defects.

## Validation

The final build's test results and checksum are recorded at the top of `BUILD_STATUS.md`. Checks cover serial startup, fonts and terminal operation, console/startx cycles, real popup menus and logout, framebuffer copies, flwm memory safety, and installation to a disposable virtual disk with subsequent disk-only boots. Visual review remains with the user.
