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

Geometry uses fixed hardware constants from `src/config.h` (LED size 3.0 mm, gap 3.5 mm, hub radius 0 mm). The renderer derives gap fraction and hub-to-arm ratio from `numLeds` and these constants — no sliders needed for geometry.

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

Pattern generation can also run less often than rendering. When that happens, the simulator reuses the last generated framebuffer and the last animation phase together. That mirrors the MCU path, where the slice scheduler keeps using the most recent rotation phase until the pattern task publishes another frame.

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

**New pattern**: Add it in `src/patterns/`, register in `src/patterns/registry.cpp`. To give it pattern-specific params, declare them in the constructor mirroring `TextPattern`. The shared JS renderer picks them up automatically.

**New timing distortion or sim-only setting**: Add an entry to `g_sim_settings[]` in `sim/settings_registry_sim.cpp` with a getter/setter wired to `TimingState` or renderer state. Apply the effect in `timing_frame()` or the renderer. The settings UI picks it up automatically — no HTML changes needed.

**New shared config field** (appears in both MCU and sim): Add an entry to `g_settings[]` in `src/settings_registry.cpp` with `Scope::Both` and getter/setter referencing the Config field. Rebuild both WASM and firmware. No JS changes needed.

**Changing a default**: The default lives exactly once — on the `Setting` entry in `src/settings_registry.cpp` (or on the `Param` member initializer inside the relevant `Pattern`/`Animation` subclass). Run `python3 tools/check_embedded_js.py` to confirm `settings_js.h` and `image_processor_js.h` are in sync with their source files — this runs as a pre-push hook.
