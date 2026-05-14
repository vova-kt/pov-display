# Perception rendering in the simulator

## The problem

Real POV hardware paints one LED slice at a time at microsecond intervals — a 360-slice revolution at 600 RPM produces one slice every ~278 us. A human watching the spinning arm sees a complete disc because their visual system integrates light over ~50–100 ms, blending thousands of individual slice flashes into a single perceived image.

A browser can't replicate this directly. At 120 fps the simulator gets one frame every ~8 ms — roughly 30x coarser than the hardware's slice rate. Rendering individual slices would require sub-frame timing the browser doesn't support.

## Two-stage integration

The simulator exploits the fact that both the monitor and the viewer's eye act as temporal integrators, and chains them together:

**Stage 1 — Monitor sample-and-hold.** Each browser frame, the shader lights only the angular wedge the arm swept during one persistence window. The LCD holds those lit pixels until the next frame overwrites them. This is the first integration — the monitor "remembers" the wedge for one frame period.

**Stage 2 — Eye temporal integration.** The human eye sums several consecutive monitor frames. At 120 Hz with a 50 ms eye integration window, the viewer's brain blends ~6 frames together. Each frame shows the arm in a different position, so the combined image covers a wide arc — recreating the partially-filled disc you'd see in person.

The real display paints thousands of fine slices that blur together in the eye. The simulator paints dozens of coarse wedges that blur together on the monitor and then in the eye. The end result looks similar because both rely on the same underlying biology.

## Angular wedge calculation

Each simulation frame, the timing model (`sim/timing.cpp`) computes two angles:

```
armAdvance = (dt / revolutionPeriod) * 360°    — how far the arm moved this frame
armSweep   = (persistenceMs / revolutionPeriod) * 360°  — how wide the lit wedge should be
```

`persistenceMs` is `1000 / displayHz` — the time one pixel stays lit on the observer's monitor. At 120 Hz display that's 8.3 ms; at 60 Hz it's 16.7 ms.

The fragment shader (`sim/renderer.cpp`) then checks each pixel: compute its polar angle, find the nearest arm, and test whether the angle falls within `armSweep` radians behind that arm. Pixels inside the wedge sample the framebuffer; pixels outside go black. With N arms, the shader uses `mod(angleBehind, TAU / N)` to repeat the wedge N times around the circle.

## Display Hz vs browser frame rate

These are separate concerns:

- **Browser frame rate** determines how smoothly the arm moves across the screen — how many distinct arm positions per second. Higher is smoother, but doesn't change how much of the disc appears lit.
- **Display Hz** (the selector in the UI) determines the wedge width — how much of the circle is lit in each frame. It models the observer's monitor refresh rate: a 60 Hz monitor holds each frame for 16.7 ms, so the wedge covers whatever arc the arm sweeps in 16.7 ms. A 240 Hz monitor refreshes 4x faster, so each wedge is 4x narrower.

This decoupling means the visual appearance stays consistent regardless of the actual browser frame rate. A browser running at 60 fps with Display Hz set to 120 shows the same wedge width as one running at 144 fps with Display Hz at 120 — only the motion smoothness differs.

## Why no decay shader

Many POV simulators fake persistence with exponential pixel decay — each frame dims the previous pixels and draws new ones on top, creating a glow trail. This approach is simple but wrong in two ways:

1. **Real LEDs don't glow.** An HD107S LED is either on or off for the duration of its slice. There's no phosphor, no afterglow. The perceived trail is entirely in the observer's eye, not emitted by the hardware.
2. **Decay rate is observer-dependent.** Eye integration time varies with brightness, retinal location, and individual physiology. Baking a fixed decay constant into the shader conflates the display's behavior with the viewer's biology.

The wedge approach avoids both problems. The shader renders exactly what the hardware emits (lit pixels in a narrow arc), and the observer's perception is reproduced naturally by the monitor + eye integration chain.

## Radial brightness falloff

Every LED on the arm emits the same luminous intensity, but LEDs at different radii sweep arcs of different length in the same time. Linear velocity is `v = ω × r`, so the arc length an LED covers during one slice is proportional to its radius. Inner LEDs deposit the same photons over a shorter arc — making them appear brighter to the observer. Perceived brightness per unit area scales as **1 / r**, normalized to the outer edge.

The fragment shader applies this correction per LED: it computes the center radius of each LED in normalized coordinates (`ledCenter = hubFrac + (led + 0.5) × ledHeight`) and multiplies the color by `1.0 / ledCenter`. With hub fraction near zero and many LEDs, the innermost LEDs can be an order of magnitude brighter than the outermost — this matches reality, where the center of a POV display is visibly brighter and can appear washed-out.

Values exceeding 1.0 are clamped by the GPU, which causes saturation (hue shift toward white) on the brightest inner pixels. This is a reasonable visual approximation of the real effect, where the inner region of a spinning display often appears overexposed.

## Practical takeaway

- **Display Hz** is the main perception knob. Lower values (60 Hz) show a fuller, more solid-looking disc — matching what you'd see on a slow monitor. Higher values (240 Hz) show a narrower arc, closer to what a fast monitor reveals.
- The simulator looks most realistic at 120 Hz Display Hz with a monitor actually running at 120 Hz or above — the two integration stages align.
- On a 60 Hz monitor, setting Display Hz above 60 won't look right — the browser can't render wedges faster than it can draw frames. Keep Display Hz at or below your actual monitor refresh.
