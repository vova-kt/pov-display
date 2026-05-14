# Power budget & runtime estimates

How to estimate battery life for the POV display. The pack is 3S1P using Samsung INR18650-35E cells (3500 mAh, 8 A max continuous, 3.6 V nominal per cell → 10.8 V nominal, 12.6 V fully charged).

## Consumers

The system has three power consumers, each drawing from the 3S pack (10.8 V nominal) through different paths.

### Motor (direct from 3S)

The motor is the dominant load. A small brushless spinning a POV arm at 1000–1500 RPM typically draws **0.5–3 A from the 3S pack** depending on arm weight, diameter, and aerodynamic drag. This is highly variable — measure yours with a watt meter on the battery lead during spinning.

For estimation, assume **1.5 A average** for a well-balanced 30 cm arm at 1200 RPM.

### LEDs (5 V rail via buck converter)

HD107S draws up to 60 mA per LED at full white, full brightness.

| Scenario | Current at 5 V | Power at 5 V |
|----------|---------------|-------------|
| 40 LEDs, full white, brightness 31 | 2.4 A | 12.0 W |
| 40 LEDs, full white, brightness 16 | ~1.2 A | ~6.2 W |
| 40 LEDs, typical pattern (30% avg) | ~0.7 A | ~3.6 W |
| 40 LEDs, dim pattern (10% avg) | ~0.24 A | ~1.2 W |

The "typical pattern" estimate assumes mixed colors at moderate brightness — rainbow, scanner, and text patterns average roughly 30% of max current. Solid white is the worst case.

**Referred back to 3S battery** through the buck converter (~90% efficiency):

```
I_battery = (I_5v × 5 V) / (10.8 V × 0.9)

Typical pattern: (0.7 × 5) / (10.8 × 0.9) ≈ 0.36 A from 3S
Full white:      (2.4 × 5) / (10.8 × 0.9) ≈ 1.24 A from 3S
```

### ESP32-C6 (5 V rail via buck converter)

The XIAO ESP32-C6 draws ~80 mA average at 3.3 V with WiFi active, which is ~130 mA from the 5 V pin (accounting for the onboard LDO). Through the buck converter:

```
I_battery = (0.13 × 5) / (10.8 × 0.9) ≈ 0.067 A from 3S
```

Negligible compared to motor and LEDs.

## Total draw from 3S

| Scenario | Motor | LEDs | MCU | Total from 3S |
|----------|-------|------|-----|---------------|
| Spinning, typical pattern | 1.5 A | 0.36 A | 0.07 A | **~1.9 A** |
| Spinning, full white | 1.5 A | 1.24 A | 0.07 A | **~2.8 A** |
| Spinning, dim pattern | 1.5 A | 0.12 A | 0.07 A | **~1.7 A** |
| Stationary (motor off) | 0 A | 0.36 A | 0.07 A | **~0.4 A** |

## Runtime formula

```
Runtime (hours) = Battery capacity (Ah) / Total draw (A)
```

### Runtime estimates — 3S1P Samsung 35E (3500 mAh)

The 35E is rated 3500 mAh at 0.2C (0.67 A) discharge. At higher currents capacity drops slightly — expect ~3300 mAh usable at 2 A, ~3100 mAh at 3 A. Using ~3200 mAh as a practical midpoint:

| Scenario | Draw | Runtime |
|----------|------|---------|
| Typical pattern | 1.9 A | **~1 h 41 min** |
| Full white | 2.8 A | **~1 h 7 min** |
| Dim pattern | 1.7 A | **~1 h 53 min** |
| Stationary (motor off) | 0.4 A | **~8 h** |

All scenarios are well within the 35E's 8 A continuous limit (worst case is 2.8 A = 0.8C).

## Alternative: USB PD powerbank

A "fat" USB-C powerbank with Power Delivery can replace the 3S 18650 pack entirely. The idea: negotiate 12 V from the powerbank via a PD trigger board, feed it into the same wiring — ESC direct, buck converter to 5 V. Nothing else changes.

### Why it works

PD 3.0 powerbanks commonly support 5 / 9 / 12 / 15 / 20 V output profiles. The 12 V profile lands right in the 3S operating range (10.8 V nominal, 12.6 V full), so the ESC and buck converter accept it without modification. The voltage is fixed at 12.0 V rather than sagging from 12.6 → 7.5 V over discharge — the buck converter is more efficient at a stable input, and the motor gets consistent power throughout the run.

### PD trigger board

The powerbank's USB-C port defaults to 5 V. To get 12 V you need a PD trigger (also called PD decoy) — a tiny board that negotiates the requested voltage during the USB-C handshake. Common options:

- **CH224K** — ~$1, set voltage via resistor or I2C. Widely available on breakout boards.
- **IP2721** — similar, resistor-selectable.
- **ZY12PDN** — pre-built module with a display, useful for prototyping.

Wire the trigger board's VBUS/GND output to an XT60 male connector (matching the system's XT60 female inlet). Add a polyfuse (3–5 A) between the trigger board and the connector — the powerbank has internal protection, but the trigger board doesn't. To swap sources, unplug one XT60 and plug in the other. See `docs/wiring-and-parts.md` for the full connector topology.

### Capacity and runtime

A 20,000 mAh powerbank stores energy at its internal cell voltage (3.7 V nominal), not at the negotiated output voltage. Delivered capacity at 12 V:

```
Delivered Ah at 12 V = (20,000 mAh × 3.7 V) / (12 V × η)

With η ≈ 0.90 (internal DC-DC):
  = (20 × 3.7) / (12 × 0.9)
  ≈ 6.9 Ah at 12 V
```

Compare to the 3S1P 35E pack's ~3.2 Ah usable — the 20 Ah powerbank delivers roughly **2× the energy**.

| Scenario | Draw | Runtime (20 Ah PB) | Runtime (3S1P 35E) |
|----------|------|---------------------|---------------------|
| Typical pattern | 1.9 A | **~3 h 38 min** | ~1 h 41 min |
| Full white | 2.8 A | **~2 h 28 min** | ~1 h 7 min |
| Dim pattern | 1.7 A | **~4 h 3 min** | ~1 h 53 min |

### Power budget check

Most 12 V PD profiles supply 3 A (36 W). Worst-case system draw is 2.8 A × 12 V = 33.6 W — fits within 36 W with ~7% headroom. Typical draw (1.9 A = 22.8 W) is well within budget. If the powerbank supports PPS (Programmable Power Supply) at 11–12 V / 5 A, there's even more margin.

### Tradeoffs vs 3S 18650 pack

| | 3S1P 18650 | 20 Ah PD powerbank |
|---|---|---|
| Energy | ~37.8 Wh (3.5 Ah × 10.8 V) | ~74 Wh |
| Weight | ~150 g (cells) + BMS | ~400–500 g |
| Volume | Custom pack, compact | Brick form factor |
| BMS / charging | Must build or buy 3S BMS + charger | Built-in, charge from any USB-C |
| Voltage profile | Sags 12.6 → 7.5 V over discharge | Fixed 12.0 V until empty |
| Extra parts | BMS, balance charger | PD trigger board (~$1), fuse |
| Failure modes | Cell imbalance, over-discharge if BMS fails | PD negotiation failure (falls back to 5 V — ESC won't arm, safe fail) |
| Swappability | Swap the pack | Swap any PD powerbank |
| Auto-shutoff | N/A | Some powerbanks shut off below ~50–100 mA — not an issue at 1.7+ A operating draw, but may cut out if motor is off and LEDs are dim |

### When to pick which

- **Powerbank** if you want long runtime, no custom battery work, and don't mind the extra weight. Good for demos and bench testing. Dead simple to set up — buy a trigger board, wire to an XT60 male.
- **18650 pack** if you need compact size, light weight, or are building the display into a finished enclosure where form factor matters.

Both plug into the same XT60 female inlet on the system side, so swapping takes seconds. Both live on the stationary side — weight doesn't affect arm balance or motor load. During a swap the system reboots briefly (config persists in NVS).

## Practical notes

- **Measure the motor.** It dominates the budget and varies wildly. A watt meter on the battery lead during spinning gives the real number. Everything else is predictable.
- **18650 discharge curve.** The 35E holds ~3.5 V per cell (10.5 V pack) for most of the discharge, then drops off steeply below 3.2 V. The buck converter handles the full range (8–12.6 V input). The BMS should cut off at ~2.5 V/cell (7.5 V pack) — the 35E datasheet allows discharge to 2.65 V.
- **18650 vs LiPo.** The 35E trades power density for energy density — heavier per watt than a LiPo pouch cell, but more mAh per dollar and easier to source. At under 3 A draw the 35E is loafing; a LiPo would be overkill on C-rating.
- **Weight.** Three 35E cells weigh ~150 g total (50 g each). The pack sits on the stationary side, so weight doesn't affect arm balance or motor load.
- **Brightness is the biggest knob.** Dropping from brightness 31 to 16 roughly halves LED power draw and the display is still bright enough indoors. The web UI makes this adjustable at runtime.
