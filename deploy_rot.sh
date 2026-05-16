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
    echo "==> Compiling rotating firmware..."
    pio run -e rotating

    echo "==> Flashing rotating firmware..."
    pio run -e rotating -t upload
fi

if [ "$MONITOR" = true ]; then
    echo "==> Opening serial monitor (Ctrl-C to exit)..."
    pio device monitor
fi
