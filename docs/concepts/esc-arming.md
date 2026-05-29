# ESC arming & calibration

How the ESC boots, why it beeps, and why we calibrate on every power cycle.

## Hardware

- **Motor**: XXD A2217 KV950 — brushless outrunner, 950 RPM/V, 85 g. Rated for 11-inch props on 3S. At POV speeds (~360 RPM) it's loafing at well under 10% throttle.
- **ESC**: XXD 30A (model FKM010) — 2–3S, 50 Hz PWM, standard 1000–2000 µs servo protocol. No BEC used (buck converter supplies 5 V). The motor coils act as a speaker — beep tones are the ESC pulsing the windings.

## What the ESC expects at boot

Standard ESC startup: the ESC powers on, checks battery voltage, then looks for the throttle signal.

1. **Voltage check** — verifies cell count is within range. If OK: three rising tones (♪ 1-2-3).
2. **Signal detection** — looks for a valid 50 Hz PWM pulse.
3. **Minimum throttle confirmation** — pulse must be at the calibrated minimum for the ESC to arm. On success: N beeps (cell count), then one long beep (ready).

If any step fails, the ESC enters an error loop instead of arming.

## The calibration problem

This ESC doesn't persist its throttle range across power cycles. Out of the box, the ESC's idea of "minimum" doesn't match our 1000 µs signal, so it refuses to arm and beeps continuously.

**Calibration** teaches the ESC the throttle range: send max pulse (2000 µs) first, then min pulse (1000 µs). The ESC records these endpoints and arms normally.

Since calibration is lost on every power-off, the firmware runs it on every boot. See `Motor::arm()` in `src/motor.cpp`.

## Boot sequence (firmware side)

```
 0 ms    MCU powers on, starts booting
~335 ms  LEDC attached on ESC pin, duty=0 (no pulse — ESC sees "disconnected")
         Immediately send 2000 µs (max throttle) for 500 ms
         ↳ ESC detects max, registers throttle range upper bound
         ↳ Motor may briefly twitch but 500 ms limits spin-up
~835 ms  Switch to 1000 µs (min throttle) for 4 s
         ↳ ESC records min, completes calibration, arms
~4.8 s   Arm complete. Firmware applies saved throttle (default 1200 µs = ~20%)
```

**Why 2000 µs must come first**: if the ESC sees only 1000 µs, it doesn't recognise the throttle range (calibration doesn't persist on this ESC) and enters continuous error beeping. The 2000→1000 sequence teaches it the range on every boot.

**Why only 500 ms at max**: the ESC registers the max value within a few PWM cycles. Keeping it short limits how much the motor spins up during this phase. The motor has significant inertia and won't reach dangerous speed in 500 ms unloaded, but keep fingers away from the arm during power-on.

**Why duty=0 before 2000 µs**: the LEDC starts with duty=0 (pin LOW for the full period — no rising edge). The ESC interprets this as "no signal," not "zero throttle." This brief no-signal gap is harmless; the ESC waits for the first valid pulse.

## Beep codes

### Normal startup (calibration + arm)

| Phase | Sound | Meaning |
|-------|-------|---------|
| ♪ 1-2-3 | Three rising tones | Voltage OK, ESC powered on |
| N short beeps | e.g. 3 beeps for 3S | Detected LiPo cell count |
| One long beep | ♪ beep——— | Armed, ready to spin |

### Errors (repeating instead of startup sequence)

| Pattern | Interval | Meaning | Likely cause |
|---------|----------|---------|-------------|
| beep-beep-, beep-beep-, … | ~1 s (pairs) | Abnormal input voltage | Battery too low/high, bad connection |
| beep-, beep-, beep-, … | ~2 s | No throttle signal | Broken wire to D3, LEDC not running |
| beep-, beep-, beep-, … (rapid) | ~0.25 s | Throttle not at minimum | Calibration failed — ESC doesn't recognize 1000 µs as minimum |

If you hear repeating beeps without the ♪ 1-2-3 startup tones, the ESC never completed voltage check or signal detection. Check power and wiring first.

### In-flight alarms

| Behavior | Meaning |
|----------|---------|
| Gradual power reduction | Low voltage cutoff (soft) — land soon |
| Immediate motor stop | Low voltage cutoff (hard) — battery critically low |
| Output cut, resumes on signal | Signal loss — no PWM for > 0.25 s |

## PWM signal details

The ESC expects standard servo-style PWM: 50 Hz (20 ms period), pulse width 1000–2000 µs.

The ESP32-C6 LEDC generates this at 16-bit resolution. At 80 MHz APB clock, the fractional divider (24 + 106/256) gives exactly 50 Hz — no frequency drift. One duty tick = 0.305 µs, so the 1000 µs pulse resolves to 3276 ticks (999.76 µs actual). Well within ESC tolerance.

Throttle mapping: 1000 µs = stop, ~1150 µs = spin threshold, 1200 µs = default (~20%, slow spin for POV), 2000 µs = full. The `escPulseUs` setting in the web UI controls the operating point. Values below ~1150 µs fall in the ESC's deadband and won't spin the motor.

## Swapping ESCs

Different ESCs may persist calibration and arm normally with a simple 1000 µs hold. If you switch to an ESC that doesn't need per-boot calibration, the 2000→1000 sequence is harmless — it just recalibrates each time. No code change needed.

If a different ESC doesn't tolerate the 2000 µs first (enters programming mode instead of calibration), the `arm()` function in `src/motor.cpp` will need adjustment.
