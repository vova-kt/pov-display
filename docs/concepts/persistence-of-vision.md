# Persistence of vision (POV)

## What it is

The human visual system doesn't "reset" between frames. When a bright stimulus disappears, the photoreceptor response decays over roughly 20-100 ms depending on brightness and retinal location. A spinning arm of LEDs exploits this: if the arm completes a full revolution within the eye's integration window, the brain perceives a continuous disc of light instead of a sweeping line.

## Why RPM is the critical variable

At 600 RPM one revolution takes 100 ms — right at the edge of the eye's integration window. The image appears mostly solid but may flicker in peripheral vision. At 1200 RPM (50 ms/rev) the image looks stable. Below ~300 RPM (200 ms/rev) you clearly see the arm sweeping with a fading phosphor trail; the POV illusion breaks down.

The relationship is straightforward: the eye integrates light over a roughly fixed time window. Higher RPM packs more of the circle into that window, so more of the image is simultaneously visible.

## Duty cycle and perceived brightness

Each LED fires for one slice interval per revolution — at 360 slices that's 1/360th of the rotation period. The eye averages this over its integration time. Perceived brightness is approximately:

```
perceived ≈ peak_brightness × (revolutions_in_persistence / num_slices)
```

At 600 RPM with 50 ms persistence and 360 slices: `(0.5 / 360) ≈ 0.14%` of peak. This is why POV displays need very bright LEDs (HD107S at 31-level hardware brightness helps, but the physics are brutal).

## How the simulator models this

The simulator does **not** fake persistence with shader-based exponential decay. Instead it renders exactly what the hardware does: each animation frame, only the angular wedge the arm swept since the last frame is lit. Everything else is black. The monitor's sample-and-hold pixel behavior acts as the first integration stage, and the viewer's eye provides the second.

On a 120 Hz display at 600 RPM:
- 12 frames per revolution, each covering a 30 degree wedge
- The eye integrates ~6 frames (~50 ms) simultaneously
- Result: ~180 degrees of arc visible at any instant, fading toward the trailing edge

This means the simulator's visual fidelity depends on monitor refresh rate — a 60 Hz display shows half as many wedges per revolution as 120 Hz, producing a coarser approximation. The real display, painting one slice at a time at microsecond intervals, is far finer-grained.

## Practical takeaway

- Target RPM >= 600 for a viewable image; >= 900 for a solid-looking one.
- More slices per revolution doesn't help perception — it determines angular resolution, not brightness or solidity. Those are governed by RPM alone.
- The simulator on a 120 Hz monitor gives a rough but honest sense of how solid the image will look at a given RPM. On 60 Hz it will look worse than reality.
