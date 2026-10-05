# Multiple Dependencies Installer
# First, detecting the distro and it will choose installation package manager command for these distro.

#!/usr/bin/env bash
set -euo pipefail

# Detect the distribution ID
if [ -f /etc/os-release ]; then
    # Load ID and ID_LIKE variables
    . /etc/os-release
    DISTRO="${ID}"
    DISTRO_LIKE="${ID_LIKE:-""}"
else
    echo "Error: /etc/os-release file not found. Cannot detect distribution." >&2
    exit 1
fi

echo "Detected distribution: $DISTRO"

# Execute installation based on the distribution
case "$DISTRO" in
    ubuntu|debian|mint|pop)
        echo "Installing build dependencies for Debian/Ubuntu-based system..."
        sudo apt-get update
        sudo apt-get install -y build-essential cmake ninja-build git pkg-config \
            libwayland-dev libxkbcommon-dev wayland-protocols \
            xorg-dev libgl1-mesa-dev
        ;;
        
    fedora|rhel|centos)
        echo "Installing build dependencies for Fedora/RHEL-based system..."
        sudo dnf install -y gcc-c++ cmake ninja-build git wayland-devel libxkbcommon-dev \
            libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel
        ;;
        
    arch|manjaro|endeavouros)
        echo "Installing build dependencies for Arch Linux-based system..."
        sudo pacman -Syu --needed base-devel cmake ninja git pkgconf wayland libxkbcommon wayland-protocols xorg-server mesa
        ;;
        
    *)
        # Fallback check using ID_LIKE for derivatives if direct match fails
        if [[ "$DISTRO_LIKE" == *"debian"* || "$DISTRO_LIKE" == *"ubuntu"* ]]; then
            echo "Installing build dependencies for Debian/Ubuntu derivative..."
            sudo apt-get update
            sudo apt-get install -y build-essential cmake ninja-build git pkg-config \
                libwayland-dev libxkbcommon-dev wayland-protocols \
                xorg-dev libgl1-mesa-dev
        elif [[ "$DISTRO_LIKE" == *"fedora"* || "$DISTRO_LIKE" == *"rhel"* ]]; then
            echo "Installing build dependencies for Fedora derivative..."
            sudo dnf install -y gcc-c++ cmake ninja-build git wayland-devel libxkbcommon-dev \
                libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel
        elif [[ "$DISTRO_LIKE" == *"arch"* ]]; then
            echo "Installing build dependencies for Arch Linux derivative..."
            sudo pacman -Syu --needed base-devel cmake ninja git pkgconf wayland libxkbcommon wayland-protocols xorg-server mesa
        else
            echo "Error: Unsupported or unrecognized distribution: $DISTRO" >&2
            exit 1
        fi
        ;;
esac

echo "All dependencies have been successfully installed!"
