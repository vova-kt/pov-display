# Architecture

## Why dual-core split

WiFi on ESP32 uses interrupts that can stall a task for several milliseconds. POV rendering needs sub-100µs slice timing. Pinning WiFi+web to Core 0 and all rendering to Core 1 eliminates cross-interference. This is the standard pattern for real-time ESP32 applications.

## Why hardware timer + deferred task (not a tight loop)

A `while(true)` spin-loop would monopolize Core 1 and prevent the pattern task from running. A hardware timer ISR fires per-slice and notifies the render task via `xTaskNotifyGiveFromISR` — the render task wakes, sends one SPI frame, then sleeps until the next notification. This leaves CPU time for pattern generation between slices.

## Why double-buffered framebuffer

Patterns take multiple milliseconds to regenerate a full frame (360 slices × N LEDs). Without double-buffering, the renderer would read partially-written data, causing tearing. The back buffer absorbs writes; `swap()` flips a single index byte — effectively atomic on ESP32.

Buffers are allocated with `MALLOC_CAP_DMA` so SPI can DMA directly from them without an intermediate copy.

## Why embedded HTML (not SPIFFS)

SPIFFS requires a separate flash partition, upload step, and filesystem overhead. The web UI is ~10 KB of HTML/CSS/JS — trivially fits in a PROGMEM string. One `pio run -t upload` deploys everything. If the UI grows past ~50 KB, move to SPIFFS.

## Why AsyncWebServer (not ESP-IDF HTTP server)

`ESPAsyncWebServer` handles requests without blocking — important since it shares Core 0 with WiFi. The ESP-IDF HTTP server works but requires more boilerplate for the same result. AsyncWebServer is the de facto standard in the Arduino-ESP32 ecosystem.

## Timing budget

At 1200 RPM / 360 slices / 36 LEDs @ 20 MHz SPI:
- Slice interval: ~139 µs
- SPI transfer: ~60 µs
- Headroom: ~79 µs

At 144 LEDs the SPI transfer exceeds the slice interval — reduce to ~180 slices or increase SPI clock. The system doesn't auto-adjust yet; this is a manual tradeoff via the web UI.

## Source map

Entry point and task creation: `src/main.cpp`
Configuration + NVS: `src/config.h`, `src/config.cpp`
Framebuffer: `src/framebuffer.h`, `src/framebuffer.cpp`
SPI LED driver: `src/hal_spi_leds.h`, `src/hal_spi_leds.cpp`
Hall sensor: `src/hall_sensor.h`, `src/hall_sensor.cpp`
Slice scheduler: `src/slice_scheduler.h`, `src/slice_scheduler.cpp`
Patterns: `src/patterns/`
Motor ESC: `src/motor.h`, `src/motor.cpp`
Web server + UI: `src/web/`
