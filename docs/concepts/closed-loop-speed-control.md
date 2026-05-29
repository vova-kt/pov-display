# Closed-loop speed control

Closed-loop control means the firmware measures the result of a command and corrects the next command from that measurement. For motor speed, the command is ESC pulse width and the measurement is Hall-derived RPM.

## Why it matters here

Brushless ESC throttle is not a calibrated speed command. The same pulse can produce very different RPM depending on battery voltage, arm drag, bearing friction, ESC calibration, and motor load. A one-time linear pulse mapping can therefore ask for 12 Hz but settle much faster.

The Hall sensor already gives one pulse per revolution for image timing. Reusing that signal for throttle feedback lets the firmware chase the requested refresh rate instead of trusting an assumed ESC curve.

## Practical takeaway

Treat refresh rate as the user-facing target and ESC pulse width as an internal actuator value. Start with a conservative pulse, cap the requested RPM with `HW_MAX_RPM` for the assembled hardware, then adjust from measured RPM until `RPM × arm_count / 60` matches the capped target. If Hall feedback is absent or stale, treat measured RPM as zero, ramp slowly, and cap the open-loop pulse so a disconnected or intermittent sensor does not cause unbounded spin-up.
