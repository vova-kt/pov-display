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
  timing.h / timing.cpp ← hardware timing simulation (C++, compiled to WASM)
  renderer.h / renderer.cpp ← WebGL 2 polar renderer with multi-arm sweep
  index.html            ← single-page UI
  js/
    wasm-engine.js      ← loads WASM, typed JS wrapper over C exports
    main.js             ← animation loop, control wiring, HUD overlay
```

### Why the timing model is in C++

The timing model mathematically simulates the scheduling pipeline: hall sensor triggers → slice interval calculation → SPI transfer budget check. Running in C++ alongside the pattern code means it compiles to WASM and shares types with the rest of the sim — no JS/C++ boundary crossing for timing state.

### Rendering approach

WebGL 2 fragment shader maps each canvas pixel to polar `(slice, led)` coordinates and samples the WASM framebuffer texture directly — no CPU-side pixel loop. The canvas auto-sizes at `devicePixelRatio` for crisp HiDPI rendering. The shader supports 1, 2, or 4 arms via `mod(angleBehind, TAU/numArms)` — no loop needed.

Geometry is driven by physical mm values (LED size 3.0 mm, gap 3.5 mm, hub radius 0 mm). The renderer derives gap fraction and hub-to-arm ratio so the display matches the real strip spacing. Computed readouts (pitch, arm radius) update live as sliders move.

A game-style HUD overlay shows motor RPM, effective refresh Hz (RPM × arms / 60), and render FPS in the top-right corner of the canvas.

### Refresh rate and arm count

Instead of setting RPM directly, the simulator uses **refresh rate** (12, 24, 25, 30, 60 Hz) and **arm count** (1, 2, 4). Motor RPM is derived: `RPM = refreshRate × 60 / numArms`. This matches how you'd design a real POV display — pick a target flicker-free rate, then choose the arm count to keep motor speed practical.

Hardware arm configurations:
- **1 arm** — one LED strip, length = radius. Balanced by a second arm without LEDs.
- **2 arms** — one LED strip, length = diameter (passes through center).
- **4 arms** — two LED strips, each = diameter (forming a +).

### POV perception model

The renderer lights only the angular wedge each arm swept during one "persistence frame." The arm angle advances by `dt / revolutionPeriod × 360°` per simulation frame, but the visible arc width is based on the **Display Hz** setting (default 120 Hz), not the browser frame rate. This decoupling keeps the visual appearance consistent across monitors of different refresh rates.

With N arms, the circle fills N× faster per revolution. A **Display Hz** selector (60, 120, 144, 240 Hz) controls the observer persistence — lower Hz = wider arc per frame = fuller disc appearance, higher Hz = narrower arc = more visible sweep. See [persistence of vision concept](concepts/persistence-of-vision.md) for the physics and [perception rendering concept](concepts/perception-rendering.md) for how the simulator reproduces it.

### Render throttling

The render loop uses `requestAnimationFrame` but only calls into the WASM sim/renderer when enough wall-clock time has elapsed for the current Display Hz setting. Between renders, simulation time still accumulates so the arm angle stays correct — the accumulated dt is passed as a single chunk when the next render fires. This avoids wasting GPU draws on frames the observer model wouldn't distinguish anyway, and makes the HUD "Render FPS" reflect actual rendered frames rather than the browser's native refresh rate.

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

**New timing effect**: Add a parameter to `TimingState` in `sim/timing.h`, wire a slider in `index.html`, apply the effect in `timing_frame()`.

**New config field**: Add a setter in `sim/sim_bridge.cpp` (e.g. `sim_set_foo()`), export it in `build.sh`, wrap it in `js/wasm-engine.js`, wire the UI control.

**Changing a default**: `src/config.h` is the source of truth. Update the Config struct initializer there, then update all consumer files (`sim/timing.h`, `sim/index.html`, `sim/js/main.js`, `src/web/web_ui.h`, `src/config.cpp`). Run `python3 check_defaults.py` to verify — it also runs as a pre-push hook.
