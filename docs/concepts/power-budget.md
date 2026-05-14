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

## Practical notes

- **Measure the motor.** It dominates the budget and varies wildly. A watt meter on the battery lead during spinning gives the real number. Everything else is predictable.
- **18650 discharge curve.** The 35E holds ~3.5 V per cell (10.5 V pack) for most of the discharge, then drops off steeply below 3.2 V. The buck converter handles the full range (8–12.6 V input). The BMS should cut off at ~2.5 V/cell (7.5 V pack) — the 35E datasheet allows discharge to 2.65 V.
- **18650 vs LiPo.** The 35E trades power density for energy density — heavier per watt than a LiPo pouch cell, but more mAh per dollar and easier to source. At under 3 A draw the 35E is loafing; a LiPo would be overkill on C-rating.
- **Weight.** Three 35E cells weigh ~150 g total (50 g each). The pack sits on the stationary side, so weight doesn't affect arm balance or motor load.
- **Brightness is the biggest knob.** Dropping from brightness 31 to 16 roughly halves LED power draw and the display is still bright enough indoors. The web UI makes this adjustable at runtime.
