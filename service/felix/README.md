# Felix service

The current deliverable is the Linux ISO described in the root `README.md`.
The latest user request replaces the smolBSD/NetBSD kernel and pkgin with
Linux 6.18 x86 and a compatible Linux package repository.

`patches/flwm-config.h` remains the release's real compile-time flwm configuration.
The active boot entry point is `userland/linux/init`, and the desktop session is
`userland/bin/felix-session`. Use `make iso` at the project root.
