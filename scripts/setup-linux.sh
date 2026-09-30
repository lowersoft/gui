#!/usr/bin/env bash
# Installs build dependencies on Debian/Ubuntu.
# Fedora equivalent: dnf install gcc-c++ cmake ninja-build git wayland-devel libxkbcommon-devel \
#                    libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel
set -euo pipefail

sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git pkg-config \
    libwayland-dev libxkbcommon-dev wayland-protocols \
    xorg-dev libgl1-mesa-dev
