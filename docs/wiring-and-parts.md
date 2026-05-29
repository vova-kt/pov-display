# Wiring & Parts

## Board

Seeed Studio XIAO ESP32-C6 — single-core RISC-V, WiFi 6, BLE 5, USB-C. Small form factor (21×17.5 mm). Mounted on the stationary side alongside the ESC and hall sensor. Powered from the 5 V buck output.

## Parts

- **LED strip** — HD107S (APA102-compatible), 5 V, SPI clock+data. 40 LEDs on a 144 LEDs/m strip (6.5 mm pitch: 3.0 mm LED + 3.5 mm gap).
- **Hall sensor** — unipolar (e.g. A3144), one pulse per rotation. Triggers the slice scheduler to sync the image to the arm position.
- **Brushless motor** — XXD A2217 KV950, outrunner, 85 g, rated for 11-inch props on 3S. At POV speeds (~360 RPM) it runs well under 10% throttle.
- **ESC** — XXD 30A (model FKM010), 2–3S, 50 Hz PWM (1000–2000 µs). BEC disconnected — buck converter supplies 5 V. Doesn't persist throttle calibration; firmware recalibrates on every boot. See `docs/concepts/esc-arming.md`.
- **Hollow slip ring** — 4 channels minimum: 5 V power (for LEDs), GND, SPI CLK, SPI MOSI. Power channel must handle 2.5 A+.
- **XT60 connector (power inlet)** — female on the system side, male on each power source. Keyed (reverse-polarity impossible), rated 60 A continuous. Lets you swap between the 3S pack and a PD powerbank in seconds. Mount the female connector where the battery lead enters the stationary chassis.
- **3S1P 18650 pack** — 3× Samsung INR18650-35E (3500 mAh, 8 A max continuous, 10.8 V nominal) + BMS for cell balancing and over-discharge protection. XT60 male on the output. See `docs/concepts/power-budget.md` for runtime estimates.
- **PD powerbank + trigger board** (alternative source) — any 20+ Ah USB-C powerbank supporting 12 V PD. A PD trigger board (CH224K or similar, ~$1) negotiates 12 V from the USB-C port. Wire the trigger board's VBUS/GND output to an XT60 male. Add a 3–5 A polyfuse between the trigger board and the connector. See `docs/concepts/power-budget.md` for capacity comparison and runtime estimates.
- **Buck converter** — XL4015-based module, 8–36 V input → 5 V output, 5 A max. Handles the full input range from both 3S pack (7.5–12.6 V) and PD powerbank (12 V). 5 A rating leaves comfortable headroom even at full white brightness 31 (~2.53 A on the 5 V rail). Add bulk capacitance (470 µF) on the 5 V output for current spikes during full-white frames. Add a 470 µF cap on the rotating side near the LED strip's VCC to absorb transients.

## Pin map

All assignments live in `src/config.h`. Pin names are Arduino XIAO ESP32-C6 board labels.

| Pin   | Board label | Function         | Destination                        |
|-------|-------------|------------------|------------------------------------|
| D8    | SCK         | SPI CLK          | LED strip CKI (via slip ring)      |
| D10   | MOSI        | SPI MOSI         | LED strip SDI (via slip ring)      |
| D2    | D2          | Hall sensor input| Hall sensor OUT (direct, pull-up, falling) |
| D3    | D3          | ESC PWM          | ESC signal wire (direct)           |
| GND   | GND         | Common ground    | All peripherals                    |

## Power topology

Two independent power domains — one stationary, one rotating.

### Stationary side

```
3S 18650 pack ──── XT60 male ─┐
                               │
PD powerbank ── PD trigger ── XT60 male ─┘  (only one connected at a time)
                  (CH224K)        │
                               XT60 female (system inlet)
                                  │
                    ┌─────────────┤
                    │             │
              ESC power in    Buck converter (→ 5 V)
                │                ├── XIAO 5V pin
                │                └── Slip ring 5 V channel ──→ rotating side
          Motor phases A/B/C

XIAO ESP32-C6
      ├── D8/D10 (SPI) ── Slip ring ──→ LED strip (rotating arm)
      ├── D2 ── Hall sensor (direct)
      └── D3 ── ESC signal wire (direct)
```

The XT60 connector is the single power inlet. Either the 3S pack or the PD powerbank plugs in — both deliver ~12 V, so the ESC and buck converter work identically with either source. Swap by unplugging one XT60 and plugging in the other. The XIAO, hall sensor, and ESC are all on the stationary side — only power and SPI data cross the slip ring.

### Rotating side

```
Slip ring 5 V ──── LED strip VCC (with 470 µF cap to GND)
Slip ring GND ──── LED strip GND
Slip ring CLK ──── LED strip CKI (at outer tip)
Slip ring MOSI ─── LED strip SDI (at outer tip)
```

The rotating arm carries only the LED strip. Power and SPI data arrive through the slip ring — no MCU or active components on the rotating side. SPI data enters at the strip's outer tip (DI end); firmware reverses the pixel order so logical pixel 0 stays at the hub. See `STRIP_REVERSED` in `src/config.h`.

### Slip ring channels

| Channel | Signal    | Direction              | Current |
|---------|-----------|------------------------|---------|
| 1       | 5 V       | Stationary → Rotating  | 2.5 A+  |
| 2       | GND       | Common                 | 2.5 A+  |
| 3       | SPI CLK   | Stationary → Rotating  | Signal  |
| 4       | SPI MOSI  | Stationary → Rotating  | Signal  |

A hollow slip ring with 4+ channels is needed. The hollow bore lets the motor shaft or wiring pass through the center.

### Voltage converters needed

Only one: **3S → 5 V buck** on the stationary side. The XIAO and LEDs share this 5 V rail (XIAO via its 5V pin, which feeds the onboard 3.3 V regulator; LEDs via the slip ring). The ESC takes battery voltage directly — no converter needed.

## Hall sensor wiring

The hall sensor (e.g. A3144, unipolar, 3-pin) sits on the stationary frame next to a magnet on the rotor. Its output is open-collector — it only pulls low when the magnet passes, leaving the line floating otherwise. A pull-up resistor sets the idle-high voltage.

The pull-up goes to **3.3 V** (XIAO's 3V3 pin), not 5 V. The sensor runs at 5 V (A3144 needs ≥ 4.5 V), but since the output only sinks current, the pull-up determines the high level. Using 3.3 V keeps the signal safe for the ESP32-C6 GPIO, which is not 5 V tolerant.

```
5 V rail ────── Hall VCC
GND ─────────── Hall GND

XIAO 3V3 ──┐
          10kΩ  (pull-up)
            ├── Hall OUT
            └── XIAO D2
```

The firmware triggers on the **falling edge** — magnet passes → output pulls low → hall ISR fires → new rotation begins.

## Motor wiring

The XIAO, ESC, and hall sensor are all on the stationary side — direct wiring, no slip ring involved.

### ESC signal connector (XXD 30A / FKM010)

The ESC has a 3-wire servo connector.

| Wire color | Function | Connect to |
|------------|----------|------------|
| **White** | Throttle PWM signal | XIAO D3 |
| **Black** | GND | XIAO GND |
| **Red** | BEC 5 V output | **Leave disconnected** |

The red BEC wire must stay disconnected — the LED strip gets 5 V from the buck converter, and connecting both 5 V sources creates a back-feed conflict. Cut or heat-shrink the red wire to prevent accidental contact.

### ESC power and motor

```
XT60 female (+) ───── ESC power in (+)
XT60 female (−) ───── ESC power in (−)

ESC motor-out A ───── Motor phase A
ESC motor-out B ───── Motor phase B
ESC motor-out C ───── Motor phase C
```

**Phase order** — swapping any two motor wires reverses spin direction. No firmware change needed.

**Arming** — on boot the firmware sends 2000 µs (max), then 1000 µs (min) to calibrate and arm the ESC. This ESC doesn't persist calibration, so it runs every power cycle. See `docs/concepts/esc-arming.md` for the full boot sequence, beep codes, and rationale.

## Why these choices

- **XIAO ESP32-C6 over ESP32 DevKit**: Small form factor (21×17.5 mm), WiFi 6, BLE 5. Single-core is sufficient — the firmware uses cooperative FreeRTOS tasks, and the timing-critical work (SPI DMA, timer ISR) runs in hardware/interrupts regardless of core count.
- **MCU on the stationary side**: Hall sensor, ESC, and XIAO are all stationary — direct wiring, no signal integrity concerns. Only SPI data and power cross the slip ring. USB-C stays accessible for flashing and debugging without stopping the motor.
- **XT60 power inlet**: Standard RC connector, keyed, rated far above our ~3 A draw. Lets you swap between the 3S pack and a PD powerbank without rewiring. Both sources output ~12 V, so everything downstream is source-agnostic. The system loses power for a few seconds during swap — ESP32 reboots (config survives in NVS), ESC re-arms (3 s), motor spins back up. Acceptable for demos and bench use.
- **Single 3S battery (not separate 2S for LEDs)**: One battery, one BMS, one buck converter. Motor current spikes don't brown out LEDs because the buck converter + capacitor filtering isolates the 5 V rail. Simpler wiring and fewer failure points.
- **XIAO on the same 5 V rail as LEDs**: The XIAO draws ~80 mA — negligible next to the LEDs' 2.2 A. A 470 µF cap on the rotating side near the LED strip absorbs transient dips from full-white frames.
- **Hollow slip ring**: Lets the motor shaft or cable bundle pass through the center. 4 channels needed: 5 V power, GND, SPI CLK, SPI MOSI. Fewer channels = less friction, less noise, longer life.
- **HD107S over WS2812**: SPI clock means no timing-sensitive bit-bang — critical on a spinning arm where ISR jitter is unavoidable. See `docs/led-strip-and-driver.md`.
- **Hall sensor over encoder**: One magnet + one sensor gives a single sync pulse per revolution — all the POV display needs.
- **ESC + brushless over DC motor**: High RPM, low vibration, ESC handles commutation. PWM control via LEDC is trivial.
