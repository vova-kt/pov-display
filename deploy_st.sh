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
    echo "==> Regenerating embedded JS headers..."
    tools/gen_embedded_js.sh sim/js/settings_ui.js    SETTINGS_JS        src/web/settings_js.h
    tools/gen_embedded_js.sh sim/js/image-processor.js IMAGE_PROCESSOR_JS src/web/image_processor_js.h

    echo "==> Compiling stationary firmware..."
    pio run -e stationary

    echo "==> Flashing stationary firmware..."
    pio run -e stationary -t upload
fi

if [ "$MONITOR" = true ]; then
    echo "==> Opening serial monitor (Ctrl-C to exit)..."
    pio device monitor
fi
