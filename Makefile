# Run with GNU make on Debian/Linux or inside Debian WSL, as root.
SHELL := /bin/sh
.DEFAULT_GOAL := iso
# The stages share one Alpine chroot and release directory.
.NOTPARALLEL:
PYTHON ?= python3
JOBS ?= 8
export JOBS

.PHONY: all prepare kernel userland rootfs iso iso-only website help test test-blit test-shadow test-text run
all: iso

prepare:
	sh tools/prepare-build.sh

.PHONY: sources
sources:
	$(PYTHON) tools/fetch-sources.py

kernel: prepare
	sh tools/build-kernel.sh

userland: prepare
	chroot /var/tmp/felix-linux/alpine sh /src/build-alpine.sh
	$(PYTHON) tools/make-theme.py

rootfs: kernel userland
	sh tools/pack-iso.sh rootfs

iso: rootfs
	sh tools/pack-iso.sh iso-only

# Explicit fast repack of the last completed rootfs; validates its checksums.
iso-only:
	sh tools/pack-iso.sh iso-only

website:
	$(PYTHON) tools/build-web.py

test:
	$(PYTHON) tools/check-serial.py
test-text:
	$(PYTHON) tools/check-textboot.py
test-blit:
	$(PYTHON) tools/test-blit.py
test-shadow:
	$(PYTHON) tools/test-shadow.py
run:
	qemu-system-i386 -m 512 -cdrom build/release/felix-1.1-x86.iso -boot d -vga std -nic user,model=e1000 -serial stdio

help:
	@printf '%s\n' 'make / make iso : build kernel, userland, rootfs and BIOS ISO' 'make rootfs     : build initramfs.cpio.xz' 'make kernel     : build the 32-bit Linux kernel' 'make userland   : build apps, TinyX, flwm, wbar and theme assets' 'make iso-only   : repack the existing, verified rootfs' 'make website    : refresh the static site and its ISO download' 'make test       : run serial-only desktop checks' 'make test-text  : verify text boot and startx' 'Output: build/release/; Linux root privileges required for builds'
