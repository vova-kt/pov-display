POV Display — Design Doc

1. Task Definition and Constraints

1.1 Goal

Build a POV display system that renders readable visual content by rotating or moving a line/strip of LEDs fast enough that persistence of vision forms a complete image.

The system should support a minimal first version, then leave room for higher-resolution rendering, animation, better synchronization, and more robust mechanical/electrical design.

1.2 Problem Statement

A POV display has to coordinate three things precisely:

1. Position / phase — know where the LED strip is in its rotation or sweep cycle.
2. Timing — update LEDs at the correct moment for each angular or spatial slice.
3. Pixel data — convert images, text, or animations into time-indexed LED frames.

Failure in any of these creates blur, wobble, tearing, unreadable text, or unstable images.

1.3 MVP Scope

The first working version should:

* Drive a small LED strip or LED array.
* Display a simple static pattern or text.
* Use a repeatable rotation / movement mechanism.
* Synchronize rendering with at least one reference signal per cycle.
* Allow brightness and frame timing to be tuned.
* Wireless configuration.

Out of scope for MVP:

* Full-color video playback.
* High-resolution image pipeline.
* Complex enclosure design.
* Battery optimization.
* Production-grade balancing.

# Available parts:

Bambu lab A1 3d printer
Microcontroller: ESP32 wifi + bluetooth
Addressable LEDs: HD107S, 1m, 144 led/m
Motor: A2217 950KV (dimension: 27.8 x 32 mm, Weight: Shaft Diameter: 4 mm, Accessories: 4.0mm prop adapter, motor mount)
Motor driver: Hobbywing Skywalker 20A V1
Hall sensor: US5881
Ball bearings and srews and nuts
4 wires slip ring


# Future Steps — Sparse Draft

Step 1 — Define MVP Geometry

* Choose circular vs linear POV design.
* Pick LED count.
* Pick approximate radius/display size.
* Estimate target RPM and slice count.

Step 2 — Build Minimal Electrical Prototype

* Connect microcontroller to LEDs.
* Render test patterns without motor.
* Verify brightness and current draw.
* Add current limiting / safe power setup.

Step 3 — Add Position Sensing

* Mount Hall sensor or optical sensor.
* Generate one clean pulse per rotation.
* Measure pulse interval.
* Log RPM stability.

Step 4 — Implement Slice Scheduler

* Divide measured rotation period into slices.
* Output one LED column per slice.
* Add phase offset calibration.
* Start with very low slice count.

Step 5 — Mechanical Prototype

* Mount LEDs to rotor/arm.
* Route wires safely.
* Balance rotor.
* Test at low RPM behind guard.

Step 6 — First Visible Image

* Render simple line/circle pattern.
* Then render letters.
* Tune phase, brightness, slice count, and RPM.
