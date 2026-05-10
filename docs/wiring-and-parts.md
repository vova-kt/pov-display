# Wiring & Parts

## Board

ESP32 DevKit V1 (30-pin, USB-C). Any ESP-WROOM-32 devkit works — the firmware only uses standard GPIOs and VSPI.

## Parts

- **LED strip** — HD107S (APA102-compatible), 5 V, SPI clock+data. 36 LEDs default; up to 144 supported at reduced slice count.
- **Hall sensor** — unipolar (e.g. A3144), one pulse per rotation. Triggers the slice scheduler to sync the image to the arm position.
- **ESC + brushless motor** — standard RC ESC accepting 50 Hz PWM (1000–2000 µs pulse). Spins the arm.
- **Slip ring** — at least 4 channels: 2 for power (5 V, GND), 2 for SPI signals (CLK, MOSI). Power channels must handle 2 A+.
- **5 V power supply** — sized for peak LED current. 36 LEDs × 60 mA = 2.2 A worst-case (all white, full brightness). Add margin for the ESP32 (~200 mA).

## Pin map

All assignments live in `src/config.h`.

| GPIO | Board label | Function | Destination |
|------|-------------|----------|-------------|
| 18 | D18 | VSPI CLK | LED strip CKI (via slip ring) |
| 23 | D23 | VSPI MOSI | LED strip SDI (via slip ring) |
| 27 | D27 | Hall sensor input | Hall sensor OUT (pull-up, interrupt on falling edge) |
| 25 | D25 | ESC PWM | ESC signal wire |
| GND | GND | Common ground | All peripherals |
| VIN | VIN | 5 V input | Power supply (shared with strip) |

## Power wiring

The LED strip needs its own 5 V feed — the ESP32's 3.3 V regulator can't supply 2 A+.

```
5V PSU ──┬── LED strip VCC  (directly, through slip ring)
         ├── ESP32 VIN
         └── common GND ── ESP32 GND + strip GND + hall sensor GND + ESC GND
```

ESP32 data pins are 3.3 V. HD107S accepts 3.3 V logic on a 5 V rail in practice. A level shifter improves signal integrity through the slip ring but isn't strictly required at 20 MHz with short wires.

## Why these choices

- **HD107S over WS2812**: SPI clock means no timing-sensitive bit-bang — critical on a spinning arm where ISR jitter is unavoidable. See `docs/led-strip-and-driver.md`.
- **Hall sensor over encoder**: one magnet + one sensor gives a single sync pulse per revolution — all the POV display needs. An encoder would add complexity for angular precision we don't use (the timer subdivides the rotation).
- **ESC + brushless over DC motor**: brushless motors are standard for spinning POV arms — high RPM, low vibration, and ESC handles commutation. PWM control via `ledc` is trivial on ESP32. See `src/motor.cpp`.
