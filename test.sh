#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $(basename "$0") [--filter <suite>] [-v]"
    echo "  --filter <suite>  Run only the named test suite (e.g. test_framebuffer)"
    echo "  -v                Verbose output"
    exit 1
}

FILTER=""
VERBOSE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --filter)  FILTER="$2"; shift 2 ;;
        -v)        VERBOSE="-v"; shift ;;
        -h|--help) usage ;;
        *)         echo "Unknown option: $1"; usage ;;
    esac
done

cd "$(dirname "$0")"

ARGS=(-e native)
[[ -n "$FILTER" ]]  && ARGS+=(--filter "$FILTER")
[[ -n "$VERBOSE" ]] && ARGS+=("$VERBOSE")

echo "==> Regenerating embedded JS headers..."
tools/gen_embedded_js.sh sim/js/settings_ui.js    SETTINGS_JS        src/web/settings_js.h
tools/gen_embedded_js.sh sim/js/image-processor.js IMAGE_PROCESSOR_JS src/web/image_processor_js.h

echo "==> Running native tests..."
pio test "${ARGS[@]}"
