#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $(basename "$0") [--no-monitor] [--monitor-only]"
    exit 1
}

MONITOR=true
SKIP_BUILD=false

for arg in "$@"; do
    case "$arg" in
        --no-monitor)    MONITOR=false ;;
        --monitor-only)  SKIP_BUILD=true ;;
        -h|--help)       usage ;;
        *)               echo "Unknown option: $arg"; usage ;;
    esac
done

cd "$(dirname "$0")"

if [ "$SKIP_BUILD" = false ]; then
    echo "==> Compiling..."
    pio run

    echo "==> Flashing..."
    pio run -t upload
fi

if [ "$MONITOR" = true ]; then
    echo "==> Opening serial monitor (Ctrl-C to exit)..."
    pio device monitor
fi
