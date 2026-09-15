#!/bin/sh
# Backward-compatible entry point; the Makefile owns build ordering.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
exec make -C "$root" iso
