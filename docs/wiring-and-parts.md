# Wiring & Parts

## Board

Seeed Studio XIAO ESP32-C6 — single-core RISC-V, WiFi 6, BLE 5, USB-C. Small form factor (21×17.5 mm) fits on the rotating arm. Powered from the 5 V slip ring rail via the 5V pin.

## Parts

- **LED strip** — HD107S (APA102-compatible), 5 V, SPI clock+data. 40 LEDs for MVP; up to 144 supported at reduced slice count.
- **Hall sensor** — unipolar (e.g. A3144), one pulse per rotation. Triggers the slice scheduler to sync the image to the arm position.
- **ESC + brushless motor** — standard RC ESC accepting 50 Hz PWM (1000–2000 µs pulse). Spins the arm.
- **Hollow slip ring** — 3 channels minimum: 5 V power (for LEDs), GND, ESC PWM signal. Power channel must handle 2.5 A+.
- **XT60 connector (power inlet)** — female on the system side, male on each power source. Keyed (reverse-polarity impossible), rated 60 A continuous. Lets you swap between the 3S pack and a PD powerbank in seconds. Mount the female connector where the battery lead enters the stationary chassis.
- **3S1P 18650 pack** — 3× Samsung INR18650-35E (3500 mAh, 8 A max continuous, 10.8 V nominal) + BMS for cell balancing and over-discharge protection. XT60 male on the output. See `docs/concepts/power-budget.md` for runtime estimates.
- **PD powerbank + trigger board** (alternative source) — any 20+ Ah USB-C powerbank supporting 12 V PD. A PD trigger board (CH224K or similar, ~$1) negotiates 12 V from the USB-C port. Wire the trigger board's VBUS/GND output to an XT60 male. Add a 3–5 A polyfuse between the trigger board and the connector. See `docs/concepts/power-budget.md` for capacity comparison and runtime estimates.
- **Buck converter** — XL4015-based module, 8–36 V input → 5 V output, 5 A max. Handles the full input range from both 3S pack (7.5–12.6 V) and PD powerbank (12 V). 5 A rating leaves comfortable headroom even at full white brightness 31 (~2.53 A on the 5 V rail). Add bulk capacitance (470 µF) on the 5 V output for current spikes during full-white frames. Add a second 470 µF cap on the rotating side near the XIAO's 5V pin to guard against brown-outs from LED current spikes.

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
3S 18650 pack ──── XT60 male ─┐
                               │
PD powerbank ── PD trigger ── XT60 male ─┘  (only one connected at a time)
                  (CH224K)        │
                               XT60 female (system inlet)
                                  │
                    ┌─────────────┤
                    │             │
              ESC power in    Buck converter (→ 5 V)
                │                └── Slip ring 5 V channel ──→ rotating side
          Motor phases A/B/C
```

The XT60 connector is the single power inlet. Either the 3S pack or the PD powerbank plugs in — both deliver ~12 V, so the ESC and buck converter work identically with either source. Swap by unplugging one XT60 and plugging in the other.

### Rotating side

```
Slip ring 5 V ──┬── LED strip VCC
                ├── XIAO 5V pin (with 470 µF cap to GND)
                └── Common GND

XIAO ESP32-C6
      ├── D8/D10 (SPI) ── LED strip (direct, short wires)
      ├── D2 ── Hall sensor
      └── D3 ── Slip ring ── ESC signal wire (stationary side)
```

The XIAO and LEDs share the same 5 V rail from the slip ring. SPI data goes directly to the LED strip — both are on the rotating arm, so no slip ring channels needed for data. The ESC signal wire is the only control signal that crosses the slip ring.

### Slip ring channels

| Channel | Signal    | Direction              | Current |
|---------|-----------|------------------------|---------|
| 1       | 5 V       | Stationary → Rotating  | 2.5 A+  |
| 2       | GND       | Common                 | 2.5 A+  |
| 3       | ESC PWM   | Rotating → Stationary  | Signal  |

A hollow slip ring with 3+ channels is enough. The hollow bore lets the motor shaft or wiring pass through the center.

### Voltage converters needed

Only one: **3S → 5 V buck** on the stationary side. The XIAO taps the same 5 V rail as the LEDs (via its 5V pin, which feeds the onboard 3.3 V regulator). The ESC takes battery voltage directly — no converter needed.

## Motor wiring

The ESC sits on the stationary side. The PWM signal crosses the slip ring from the rotating XIAO.

```
XIAO D3 ────── Slip ring channel 3 ────── ESC signal wire
Slip ring GND ────── ESC signal GND

XT60 female (+) ───── ESC power in (+)
XT60 female (−) ───── ESC power in (−)

ESC motor-out A ───── Motor phase A
ESC motor-out B ───── Motor phase B
ESC motor-out C ───── Motor phase C
```

**BEC disconnected** — the ESC's BEC (5 V) line is left disconnected; the LED strip gets 5 V from the buck converter. Connecting both creates a back-feed conflict.

**Phase order** — swapping any two motor wires reverses spin direction. No firmware change needed.

**Arming** — on boot the firmware holds 1000 µs (stop) for 3 s so the ESC recognises a valid throttle range. See `src/motor.cpp`.

## Why these choices

- **XIAO ESP32-C6 over ESP32 DevKit**: Small enough (21×17.5 mm) to mount on the rotating arm alongside the LED strip. WiFi 6 and BLE 5 are bonuses. Single-core is sufficient — the firmware uses cooperative FreeRTOS tasks, and the timing-critical work (SPI DMA, timer ISR) runs in hardware/interrupts regardless of core count.
- **XT60 power inlet**: Standard RC connector, keyed, rated far above our ~3 A draw. Lets you swap between the 3S pack and a PD powerbank without rewiring. Both sources output ~12 V, so everything downstream is source-agnostic. The system loses power for a few seconds during swap — ESP32 reboots (config survives in NVS), ESC re-arms (3 s), motor spins back up. Acceptable for demos and bench use.
- **Single 3S battery (not separate 2S for LEDs)**: One battery, one BMS, one buck converter. Motor current spikes don't brown out LEDs because the buck converter + capacitor filtering isolates the 5 V rail. Simpler wiring and fewer failure points.
- **XIAO on the same 5 V rail as LEDs**: The XIAO draws ~80 mA — negligible next to the LEDs' 2.2 A. A 470 µF cap near the XIAO's 5V pin absorbs transient dips from full-white LED frames and prevents brown-outs. No separate battery needed on the rotating arm, which saves weight and complexity.
- **Hollow slip ring**: Lets the motor shaft or cable bundle pass through the center. Only 3 channels needed (was 4 with the old layout where SPI data crossed the slip ring). Fewer channels = less friction, less noise, longer life.
- **HD107S over WS2812**: SPI clock means no timing-sensitive bit-bang — critical on a spinning arm where ISR jitter is unavoidable. See `docs/led-strip-and-driver.md`.
- **Hall sensor over encoder**: One magnet + one sensor gives a single sync pulse per revolution — all the POV display needs.
- **ESC + brushless over DC motor**: High RPM, low vibration, ESC handles commutation. PWM control via LEDC is trivial.
