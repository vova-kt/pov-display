# Closed-loop speed control

Closed-loop control means the firmware measures the result of a command and corrects the next command from that measurement. For motor speed, the command is ESC pulse width and the measurement is Hall-derived RPM.

## Why it matters here

Brushless ESC throttle is not a calibrated speed command. The same pulse can produce very different RPM depending on battery voltage, arm drag, bearing friction, ESC calibration, and motor load. A one-time linear pulse mapping can therefore ask for 12 Hz but settle much faster.

The Hall sensor already gives one pulse per revolution for image timing. Reusing that signal for throttle feedback lets the firmware chase the requested refresh rate instead of trusting an assumed ESC curve.

## RPM prediction

The Hall sensor fires once per revolution. At 360 RPM that is one pulse every ~166 ms. Between pulses, the last measured RPM is the best available estimate. But when the motor decelerates, the next pulse arrives later than expected, and continuing to report the last measured RPM over-reports speed.

`freshHallRpm()` addresses this with elapsed-time prediction. If more time has passed since the last Hall trigger than the period implied by the last RPM (`60000 / measuredRpm`), the function estimates current RPM as `60000 / elapsedMs`. This gives a monotonically decreasing estimate during deceleration instead of holding a stale high value until the timeout threshold. See `src/motor_control.cpp`.

The timeout (`kHallFreshTimeoutMs`) is the absolute cutoff; past this point the motor is considered stopped and RPM is reported as zero. The value keeps feedback alive well below the useful operating range.

## EMA filtering

One Hall pulse per rotation means one RPM sample per rotation — roughly 6 samples per second at 360 RPM. A single noisy sample (magnet jitter, edge timing variance) can cause a large proportional correction, which the motor's inertia then overshoots.

`MotorSpeedController` applies an exponential moving average (EMA) to smooth consecutive readings:

    filteredRpm += (measuredRpm - filteredRpm) >> kEmaAlphaShift

With `kEmaAlphaShift = 2` (alpha = 0.25), each new reading moves the filtered value by 25% of the difference. Implemented with a right-shift for integer-only arithmetic on the ESP32. The filter resets to zero when measured RPM drops to zero (sensor stale or motor stopped) and seeds itself with the first non-zero reading so there is no lag on startup. See `src/motor_control.cpp`.

## Tuning parameters

All constants live in `src/config.h`. The table below explains what each one controls.

| Constant | Default | Purpose |
|---|---|---|
| `kMotorStartupPulseUs` | 1160 | First ESC pulse on start. Must exceed the minimum-to-spin threshold (empirically 1140 us). |
| `kNoHallStartupMaxPulseUs` | 1200 | Maximum pulse during blind ramp (no Hall feedback). Limits overshoot before the first Hall pulse. |
| `kNoHallRampStepUs` | 5 | Pulse increment per control tick during blind ramp. |
| `kMotorControlIntervalMs` | 100 | Time between proportional corrections. |
| `kMotorControlRpmPerUs` | 15 | RPM error per 1 us pulse correction (proportional gain divisor). Higher = less aggressive. |
| `kMotorControlMaxStepUs` | 100 | Largest single-tick pulse correction. Safety clamp. |
| `kMotorControlDeadbandRpm` | 15 | RPM error below which no correction is applied. Prevents hunting around setpoint. |
| `kHallFreshTimeoutMs` | 1500 | Hall reading declared stale (RPM = 0) after this interval with no trigger. |
| `kEmaAlphaShift` | 2 | EMA filter strength as right-shift. alpha = 1/(1 << shift). |

## Practical takeaway

Treat refresh rate as the user-facing target and ESC pulse width as an internal actuator value. Start with a conservative pulse, cap the requested RPM with `HW_MAX_RPM` for the assembled hardware, then adjust from measured RPM until `RPM × arm_count / 60` matches the capped target. If Hall feedback is absent or stale, treat measured RPM as zero, ramp slowly, and cap the open-loop pulse so a disconnected or intermittent sensor does not cause unbounded spin-up.
