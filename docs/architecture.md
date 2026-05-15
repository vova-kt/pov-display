# Architecture

## Why single-core is enough

The XIAO ESP32-C6 has one RISC-V core. The old ESP32 firmware pinned WiFi to Core 0 and rendering to Core 1 to avoid WiFi interrupts stalling slice timing. On the C6, this isolation isn't possible — but it doesn't need to be:

- **SPI transfers are DMA-driven.** Once `spi_device_transmit()` kicks off, the hardware handles the transfer. A WiFi interrupt during a DMA transfer doesn't corrupt the data.
- **The slice timer is a hardware timer.** `esp_timer` fires an ISR that posts a task notification — a few microseconds of CPU. The render task wakes, calls `sendSlice()` (which starts DMA and blocks ~60 µs), then sleeps. WiFi can run between slices.
- **FreeRTOS priorities do the rest.** The render task runs at priority 24, pattern at 10, WiFi internally at ~18–23. A WiFi burst might delay a pattern frame (cosmetic), but can't starve the render task for long because the render task's work per wake-up is tiny (memcpy + DMA start).

If WiFi jitter becomes measurable at high RPM, the fallback is to disable WiFi during spinning and re-enable on stop — a software switch, not an architecture change.

## Why hardware timer + deferred task (not a tight loop)

A `while(true)` spin-loop would monopolize the CPU and prevent the pattern task and WiFi from running. A hardware timer ISR fires per-slice and notifies the render task via `xTaskNotifyGiveFromISR` — the render task wakes, sends one SPI frame, then sleeps until the next notification. This leaves CPU time for pattern generation and WiFi between slices.

## Why double-buffered framebuffer

Patterns take multiple milliseconds to regenerate a full frame (360 slices × N LEDs). Without double-buffering, the renderer would read partially-written data, causing tearing. The back buffer absorbs writes; `swap()` flips a single index byte — effectively atomic.

Buffers are allocated with `MALLOC_CAP_DMA` so SPI can DMA directly from them without an intermediate copy.

## Why embedded HTML (not SPIFFS)

SPIFFS requires a separate flash partition, upload step, and filesystem overhead. The web UI is ~10 KB of HTML/CSS/JS — trivially fits in a PROGMEM string. One `pio run -t upload` deploys everything. If the UI grows past ~50 KB, move to LittleFS.

## Why AsyncWebServer

`ESPAsyncWebServer` handles requests without blocking — important since it shares the single core with rendering. The ESP-IDF HTTP server works but requires more boilerplate for the same result.

## Timing budget

At 1200 RPM / 360 slices / 36 LEDs @ 20 MHz SPI:
- Slice interval: ~139 µs
- SPI transfer: ~60 µs
- Headroom: ~79 µs

At 144 LEDs the SPI transfer exceeds the slice interval — reduce to ~180 slices or increase SPI clock. The system doesn't auto-adjust yet; this is a manual tradeoff via the web UI.

## Animation system

Animations are time-dependent effects applied *after* pattern generation but *before* `fb.swap()`. This ordering lets animations modify the back buffer or produce metadata (like `sliceOffset` for rotation) without interfering with pattern logic.

The design is a polymorphic registry: each animation is a subclass of `Animation` with self-describing `AnimParam` parameters. A global array `g_animations[]` in `src/animation.cpp` holds all registered animations. `applyAnimations()` iterates the registry, skipping inactive ones. Parameters live in the animation objects — not in `Config` — so adding a new animation never touches Config or NVS schema. See `src/animation.h` for the base class and `src/animations/rotation.h` for the first concrete implementation.

Adding a new animation: create a class in `src/animations/`, add one line to `src/animation.cpp`, then wire UI plumbing. The frame loops in `main.cpp` and `sim_bridge.cpp` never change because they call `applyAnimations()` generically.

## Source map

Entry point and task creation: `src/main.cpp`
Configuration + NVS: `src/config.h`, `src/config.cpp`
Framebuffer: `src/framebuffer.h`, `src/framebuffer.cpp`
SPI LED driver: `src/hal_spi_leds.h`, `src/hal_spi_leds.cpp`
Hall sensor: `src/hall_sensor.h`, `src/hall_sensor.cpp`
Slice scheduler: `src/slice_scheduler.h`, `src/slice_scheduler.cpp`
Patterns: `src/patterns/`
Canvas (rectangular pixel buffer): `src/canvas.h`, `src/canvas.cpp`
Coordinate transforms: `src/coord_transform.h`, `src/transforms/`
Motor ESC: `src/motor.h`, `src/motor.cpp`
Web server + UI: `src/web/`
