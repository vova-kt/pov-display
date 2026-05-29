# LED Strip & Driver

## HD107S protocol

HD107S is a clock-and-data addressable LED, electrically identical to APA102 / SK9822. Unlike WS2812 (single-wire, timing-sensitive), HD107S uses SPI: a clock line lets the controller set the data rate, and the protocol tolerates jitter — critical for POV where slice timing already varies. The firmware uses `SPI2_HOST` on the ESP32-C6 with DMA.

Each SPI frame on the wire looks like:

```
[Start frame: 32 zero bits]
[LED 0: 111 + 5-bit brightness + 8B blue + 8G green + 8R red]
[LED 1: ...]
...
[LED N-1: ...]
[End frame: ≥ ceil(N/2) bits of 1]
```

The 5-bit "global brightness" field (0–31) drives a constant-current sink per LED. This is separate from the 8-bit per-channel PWM and is more power-efficient at low brightness — it dims the current rather than duty-cycling the already-on LEDs. We use it as the user-facing brightness control.

**Byte order is BGR, not RGB.** The `Pixel` struct in `src/framebuffer.h` stores bytes in wire order (brightness, blue, green, red) so the driver can `memcpy` a full slice directly into the SPI buffer with no per-pixel swizzle. Pattern generators write in RGB via `setPixel()`; the conversion happens once at write time.

## Why SPI with DMA

A 36-LED slice is 4 (start) + 144 (pixels) + 3 (end) = 151 bytes. At 20 MHz that takes ~60 µs. The ESP32-C6 SPI peripheral has built-in DMA, so the CPU kicks off the transfer and is free during those 60 µs — the render task just calls `spi_device_transmit()` and blocks until DMA completes. No bit-banging, no busy-wait.

The TX buffer is allocated once at init with `MALLOC_CAP_DMA` (must be in DMA-capable SRAM). It's reused every slice — `buildFrame()` writes start frame + pixel data + end frame, then the same buffer is handed to the SPI peripheral.

## Why 20 MHz clock (not 40 MHz)

HD107S is rated to 40 MHz, but the wires on the rotating arm are subject to vibration and centrifugal stress. 20 MHz is conservative and reliable; it can be increased with the fixed `HW_SPI_CLOCK_MHZ` build setting if the wiring is clean. In the active single-MCU build, SPI crosses the slip ring from the stationary MCU to the rotating LED strip; the archived dual-MCU design kept SPI local to the arm.

## Why no chip-select pin

APA102-family LEDs don't have a CS input — they latch data on clock edges continuously. The driver sets `spics_io_num = -1` to tell the ESP32 SPI peripheral not to toggle any CS line, saving a GPIO.

## End frame sizing

The end frame propagates clock through the daisy chain. Each LED "eats" one clock edge, so after N LEDs you need at least N/2 extra bytes (each byte = 8 clock edges) of 0xFF. The formula `ceil(N/16)` bytes covers this. Undersizing causes the last LEDs to not latch; oversizing is harmless.

## Data flow

```
Pattern generator
  → setPixel(slice, led, R, G, B, brightness)    [converts RGB→wire-order Pixel]
  → back buffer (DMA-capable RAM)
  → swap()
  → front buffer
  → sendSlice() memcpy into TX buffer
  → SPI DMA → CLK/MOSI pins → LED strip
```

Source: `src/hal_spi_leds.h`, `src/hal_spi_leds.cpp`, `src/framebuffer.h`
