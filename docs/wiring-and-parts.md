# Wiring & Parts

## Board

Seeed Studio XIAO ESP32-C6 — single-core RISC-V, WiFi 6, BLE 5, USB-C. Small form factor (21×17.5 mm) fits on the rotating arm. Onboard single-cell LiPo charger and 3.3 V regulator.

## Parts

- **LED strip** — HD107S (APA102-compatible), 5 V, SPI clock+data. 40 LEDs for MVP; up to 144 supported at reduced slice count.
- **Hall sensor** — unipolar (e.g. A3144), one pulse per rotation. Triggers the slice scheduler to sync the image to the arm position.
- **ESC + brushless motor** — standard RC ESC accepting 50 Hz PWM (1000–2000 µs pulse). Spins the arm.
- **Hollow slip ring** — 3 channels minimum: 5 V power (for LEDs), GND, ESC PWM signal. Power channel must handle 2.5 A+.
- **3S LiPo + BMS** — 11.1 V nominal, powers the motor (direct to ESC) and LEDs (via buck converter). BMS handles cell balancing and over-discharge protection.
- **Buck converter** — 3S (8–12.6 V) → 5 V, 3 A+. Powers the LED strip through the slip ring. An MP1584-based module works; add bulk capacitance (470 µF) on the 5 V output for current spikes during full-white frames.
- **500 mAh LiPo cell** — single-cell (3.7 V), connects directly to the XIAO battery pads on the rotating arm. The XIAO's onboard charger tops it up over USB-C when not spinning.

## Pin map

All assignments live in `src/config.h`. Pin names are Arduino XIAO ESP32-C6 board labels.

| Pin   | Board label | Function         | Destination                        |
|-------|-------------|------------------|------------------------------------|
| D8    | SCK         | SPI CLK          | LED strip CKI (on rotating arm)   |
| D10   | MOSI        | SPI MOSI         | LED strip SDI (on rotating arm)   |
| D2    | D2          | Hall sensor input| Hall sensor OUT (pull-up, falling) |
| D3    | D3          | ESC PWM          | ESC signal wire (via slip ring)    |
| GND   | GND         | Common ground    | All peripherals                    |

## Power topology

Two independent power domains — one stationary, one rotating.

### Stationary side

```
3S LiPo ──┬── ESC power in (direct, 11.1 V)
    │      └── Motor phases A/B/C
    │
    └── Buck converter (3S → 5 V)
           └── Slip ring 5 V channel ──→ rotating side
```

The ESC takes 3S voltage directly. The buck converter steps down to 5 V for the LED strip, fed through the slip ring.

### Rotating side

```
Slip ring 5 V ──┬── LED strip VCC
                └── Common GND

XIAO ESP32-C6 ──── 500 mAh LiPo (battery pads, onboard charger)
      │
      ├── D8/D10 (SPI) ── LED strip (direct, short wires)
      ├── D2 ── Hall sensor
      └── D3 ── Slip ring ── ESC signal wire (stationary side)
```

The XIAO runs on its own single-cell LiPo. SPI data goes directly to the LED strip — both are on the rotating arm, so no slip ring channels needed for data. The ESC signal wire is the only control signal that crosses the slip ring.

### Slip ring channels

| Channel | Signal    | Direction              | Current |
|---------|-----------|------------------------|---------|
| 1       | 5 V       | Stationary → Rotating  | 2.5 A+  |
| 2       | GND       | Common                 | 2.5 A+  |
| 3       | ESC PWM   | Rotating → Stationary  | Signal  |

A hollow slip ring with 3+ channels is enough. The hollow bore lets the motor shaft or wiring pass through the center.

### Voltage converters needed

Only one: **3S → 5 V buck** on the stationary side. No converter needed for the XIAO (single-cell LiPo connects directly to battery pads with onboard regulation) or the ESC (takes battery voltage directly).

## Motor wiring

The ESC sits on the stationary side. The PWM signal crosses the slip ring from the rotating XIAO.

```
XIAO D3 ────── Slip ring channel 3 ────── ESC signal wire
Slip ring GND ────── ESC signal GND

3S LiPo (+) ───── ESC power in (+)
3S LiPo (−) ───── ESC power in (−)

ESC motor-out A ───── Motor phase A
ESC motor-out B ───── Motor phase B
ESC motor-out C ───── Motor phase C
```

**BEC disconnected** — the ESC's BEC (5 V) line is left disconnected; the LED strip gets 5 V from the buck converter. Connecting both creates a back-feed conflict.

**Phase order** — swapping any two motor wires reverses spin direction. No firmware change needed.

**Arming** — on boot the firmware holds 1000 µs (stop) for 3 s so the ESC recognises a valid throttle range. See `src/motor.cpp`.

## Why these choices

- **XIAO ESP32-C6 over ESP32 DevKit**: Small enough (21×17.5 mm) to mount on the rotating arm alongside the LED strip. Onboard LiPo charger eliminates a separate charging circuit. WiFi 6 and BLE 5 are bonuses. Single-core is sufficient — the firmware uses cooperative FreeRTOS tasks, and the timing-critical work (SPI DMA, timer ISR) runs in hardware/interrupts regardless of core count.
- **Single 3S battery (not separate 2S for LEDs)**: One battery, one BMS, one buck converter. Motor current spikes don't brown out LEDs because the buck converter + capacitor filtering isolates the 5 V rail. Simpler wiring and fewer failure points.
- **Hollow slip ring**: Lets the motor shaft or cable bundle pass through the center. Only 3 channels needed (was 4 with the old layout where SPI data crossed the slip ring). Fewer channels = less friction, less noise, longer life.
- **500 mAh LiPo on rotating arm**: Decouples the MCU power from the slip ring entirely. The XIAO draws ~80 mA average — 500 mAh gives ~5 hours of runtime. Charged via USB-C when stationary.
- **HD107S over WS2812**: SPI clock means no timing-sensitive bit-bang — critical on a spinning arm where ISR jitter is unavoidable. See `docs/led-strip-and-driver.md`.
- **Hall sensor over encoder**: One magnet + one sensor gives a single sync pulse per revolution — all the POV display needs.
- **ESC + brushless over DC motor**: High RPM, low vibration, ESC handles commutation. PWM control via LEDC is trivial.
