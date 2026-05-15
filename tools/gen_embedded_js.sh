#!/usr/bin/env bash
# Embed a JS source file as a PROGMEM C string constant.
# Usage: gen_embedded_js.sh <source.js> <OutputConstName> <output.h>
set -euo pipefail

SRC="$1"
CONST="$2"
OUT="$3"

cat > "$OUT" <<EOF
#pragma once
#include <pgmspace.h>

// Auto-generated from $SRC — do not edit directly.
// Run tools/gen_embedded_js.sh to regenerate.

static const char ${CONST}[] PROGMEM = R"rawliteral(
$(cat "$SRC")
)rawliteral";
EOF
echo "Generated $OUT"
