#!/usr/bin/env python3
"""Verify that generated PROGMEM JS headers match their source files.
Run from the project root.
"""
import sys
import re
from pathlib import Path

checks = [
    ("sim/js/settings_ui.js",    "src/web/settings_js.h",       "SETTINGS_JS"),
    ("sim/js/image-processor.js","src/web/image_processor_js.h","IMAGE_PROCESSOR_JS"),
]

fail = False
for src, hdr, const in checks:
    src_text  = Path(src).read_text()
    hdr_text  = Path(hdr).read_text()
    # Extract the embedded body between R"rawliteral( and )rawliteral"
    m = re.search(r'R"rawliteral\((.*?)\)rawliteral"', hdr_text, re.DOTALL)
    if not m:
        print(f"FAIL: {hdr} — rawliteral block not found")
        fail = True
        continue
    embedded = m.group(1)
    # Strip leading/trailing newline added by the heredoc in gen_embedded_js.sh
    embedded = embedded.strip('\n')
    source   = src_text.strip('\n')
    if embedded != source:
        print(f"FAIL: {hdr} is out of sync with {src}")
        print(f"  Re-run: tools/gen_embedded_js.sh {src} {const} {hdr}")
        fail = True
    else:
        print(f"OK: {hdr} matches {src}")

sys.exit(1 if fail else 0)
