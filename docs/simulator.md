# Browser Simulator

## Why

Testing patterns on real hardware requires: compile, flash, spin motor, eyeball the result. The simulator runs the **real C++ pattern code** in the browser via WebAssembly, with a timing model that reproduces hardware artifacts — no motor needed.

## Architecture

The firmware's pattern + framebuffer code compiles to WASM via Emscripten, using the same `test/stubs/` that native tests use. No code is duplicated or re-implemented in JS.

```
sim/
  build.sh              ← Emscripten build (auto-installs emsdk if missing)
  sim_bridge.cpp        ← C-linkage API exposing patterns/framebuffer to JS
  sim_config_stub.cpp   ← no-op NVS methods (Config::loadFromNvs/saveToNvs)
  index.html            ← single-page UI
  js/
    wasm-engine.js      ← loads WASM, typed JS wrapper over C exports
    timing-model.js     ← hardware timing simulation (all JS)
    radial-renderer.js  ← Canvas 2D polar rendering via ImageData
    main.js             ← animation loop, control wiring
```

### Why the timing model is in JS, not C++

The real firmware timing uses FreeRTOS tasks + `esp_timer` hardware timers. Porting those to WASM would mean building an RTOS simulator. Instead, the JS timing model mathematically simulates the scheduling pipeline: hall sensor triggers → slice interval calculation → SPI transfer budget check. This is accurate enough to surface the real tradeoffs (RPM vs slice count vs SPI speed) while being easy to extend with new distortion knobs.

### Rendering approach

The renderer uses an `ImageData` buffer with a precomputed polar lookup table: for each canvas pixel `(x, y)`, it stores the corresponding `(slice, led)` index. On each frame, it scans the lookup table and reads pixel data directly from WASM linear memory (zero-copy via `Module.HEAPU8`). The lookup table is rebuilt only when canvas size or LED/slice count changes.

## Timing distortions

These simulate real hardware problems and their visual effects:

| Knob | What it models | Real-world cause |
|---|---|---|
| RPM jitter | Per-revolution speed variation | Motor load, vibration, aero drag |
| Hall jitter | Trigger timing noise (µs) | Magnetic field edge detection uncertainty |
| Hall miss rate | Missed triggers | Sensor out of range, debounce too aggressive |
| Timer drift (ppm) | Cumulative slice timing error | Crystal oscillator imprecision |
| Pattern lag | Extra pattern generation time | Complex patterns, slow code paths |
| SPI clock | Transfer speed for budget calc | Wire length, signal integrity limits |

The SPI transfer time formula matches the real HD107S wire protocol: `(4 + numLeds×4 + ceil(numLeds/16)) × 8 / (spiClockMhz × 1e6)` — see `src/hal_spi_leds.cpp`.

## How to extend

**New pattern**: Add it in `src/patterns/`, register in the `patterns[]` array in `sim/sim_bridge.cpp`, rebuild WASM. The simulator picks it up automatically via `sim_num_patterns()` / `sim_pattern_name()`.

**New timing effect**: Add a parameter to `TimingModel` in `js/timing-model.js`, wire a slider in `index.html`, apply the effect in `simulateRevolution()`.

**New config field**: Add a setter in `sim/sim_bridge.cpp` (e.g. `sim_set_foo()`), export it in `build.sh`, wrap it in `js/wasm-engine.js`, wire the UI control.
