#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

EMSDK_DIR="${EMSDK:-$HOME/emsdk}"

if ! command -v emcc &>/dev/null; then
    echo "emcc not found — bootstrapping emsdk into $EMSDK_DIR ..."
    if [ ! -d "$EMSDK_DIR" ]; then
        git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
    fi
    (cd "$EMSDK_DIR" && ./emsdk install latest && ./emsdk activate latest)
    source "$EMSDK_DIR/emsdk_env.sh"
fi

EXPORTED='[
  "_sim_init","_sim_resize",
  "_sim_num_slices","_sim_num_leds",
  "_sim_set_brightness","_sim_set_color","_sim_set_phase_offset","_sim_set_text",
  "_sim_num_patterns","_sim_pattern_name",
  "_sim_renderer_init","_sim_renderer_resize",
  "_sim_renderer_set_hub_frac","_sim_renderer_set_gap_frac",
  "_sim_renderer_set_show_overruns","_sim_renderer_set_show_hall_marker",
  "_sim_renderer_set_show_slice_grid","_sim_renderer_set_num_arms",
  "_sim_timing_set_rpm","_sim_timing_set_rpm_jitter",
  "_sim_timing_set_hall_jitter","_sim_timing_set_hall_miss",
  "_sim_timing_set_timer_drift","_sim_timing_set_pattern_lag",
  "_sim_timing_set_spi_clock",
  "_sim_frame",
  "_sim_get_actual_rpm","_sim_get_slice_interval_us",
  "_sim_get_spi_transfer_us","_sim_get_headroom_us",
  "_sim_get_frame_age","_sim_get_pattern_gen_ms",
  "_sim_get_hall_missed","_sim_get_has_overruns",
  "_malloc","_free"
]'
EXPORTED=$(echo "$EXPORTED" | tr -d ' \n')

em++ -O2 \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='PovSimModule' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s "EXPORTED_FUNCTIONS=$EXPORTED" \
  -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']" \
  -s NO_EXIT_RUNTIME=1 \
  -s USE_WEBGL2=1 \
  -s FULL_ES3=1 \
  -DMAX_LEDS=144 \
  -DMAX_SLICES=720 \
  -I../test/stubs \
  -I../src \
  -I. \
  ../src/framebuffer.cpp \
  ../src/patterns/solid.cpp \
  ../src/patterns/rainbow.cpp \
  ../src/patterns/scanner.cpp \
  ../src/patterns/text.cpp \
  timing.cpp \
  renderer.cpp \
  sim_bridge.cpp \
  sim_config_stub.cpp \
  -o pov_sim.js

echo "Build complete: sim/pov_sim.js + sim/pov_sim.wasm"
echo "Serve with:  cd sim && python3 -m http.server 8080"
