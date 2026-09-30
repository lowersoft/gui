#!/usr/bin/env bash
# Installs the UCRT64 toolchain. Run once from an MSYS2 terminal.
set -euo pipefail

pacman -S --needed --noconfirm \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-gdb \
    git

# Optional, for the clang-* presets:
#   pacman -S --needed mingw-w64-ucrt-x86_64-clang

echo "Done. Next, from the MSYS2 UCRT64 terminal: ./build.sh release --run"
