#!/usr/bin/env bash
# Usage: ./build.sh [debug|release|clang-debug|clang-release] [--run]
# On Windows run it from the "MSYS2 UCRT64" terminal; on Linux from any terminal.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

if [[ -n "${MSYSTEM:-}" && "$MSYSTEM" != "UCRT64" ]]; then
    echo "error: running in the '$MSYSTEM' MSYS2 environment; this project requires 'UCRT64'." >&2
    echo "Open the 'MSYS2 UCRT64' terminal from the Start menu and run this script again." >&2
    exit 1
fi

for tool in cmake ninja g++; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: '$tool' not found. Run ./scripts/setup-msys2.sh (MSYS2) or ./scripts/setup-linux.sh first." >&2
        exit 1
    fi
done

preset="${1:-release}"
run="${2:-}"

cmake --preset "$preset"
cmake --build --preset "$preset" --parallel

if [[ "$run" == "--run" ]]; then
    exe="build/$preset/bin/hello"
    [[ -f "$exe.exe" ]] && exe="$exe.exe"
    "./$exe"
fi
