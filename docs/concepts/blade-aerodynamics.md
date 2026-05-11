# Blade aerodynamics & noise

## Why a spinning blade makes noise

A spinning blade pushes air out of the way. The noise comes from **turbulence** — chaotic, swirling air pockets that form when airflow separates from the blade surface. Two main mechanisms:

1. **Vortex shedding.** When air hits a blunt edge (like the flat side of a rectangular PCB), it can't follow the surface smoothly. It "separates" and curls into spinning vortices that peel off alternately from each side. These vortices create pressure fluctuations — which is literally what sound is. The frequency depends on speed and blade thickness, often producing a tonal whine.

2. **Turbulent boundary layer.** Even on smooth surfaces, a thin layer of air clings to the blade (the "boundary layer"). If the surface is rough or the shape changes abruptly, this layer becomes turbulent and radiates broadband noise (a whooshing/rushing sound).

## Why tips matter most

Linear velocity increases with radius: `v = ω × r`. The tips of a 28.5 cm blade spinning at 20 Hz move at **~36 m/s** (~130 km/h). Near the hub, velocity is close to zero. Since aerodynamic noise scales with roughly **v⁵ to v⁶**, the outer 30% of the blade generates the vast majority of the noise. That's why tapering and shaping the tips has an outsized effect.

## What an airfoil does

An airfoil (or "streamlined") cross-section is shaped so air flows smoothly along both sides without separating. Think of a stretched teardrop: rounded at the leading edge, tapering to a thin trailing edge. This shape:

- **Delays flow separation** — air stays attached to the surface longer, reducing vortex shedding.
- **Reduces frontal area** — less air displaced = less pressure disturbance.
- **Minimizes wake** — the turbulent region behind the blade is much smaller.

A symmetric airfoil (like NACA 0012) works well here because the blade doesn't need to generate lift — it just needs to move through air quietly. The NACA 4-digit code encodes the shape: first digit = max camber (0 = symmetric, no lift), second digit = camber position, last two = max thickness as % of chord. So "0012" = symmetric, 12% thick.

### Choosing thickness ratio for this project

The LED strip (12mm wide × 2mm tall) is the binding constraint. At a pure NACA 0012 with 15mm chord, max thickness would be only 1.8mm — the strip doesn't fit. The blade chord must widen to ~18–20mm so airfoil walls can wrap around the strip. With 20mm chord and ~5mm total thickness (strip + walls above and below), the effective ratio is ~25% (NACA 0025 territory). That's fatter than aerodynamically ideal, but still far better than a bare rectangle. At this thickness ratio the leading-edge roundness, trailing-edge taper, and tip shaping matter more than matching a specific NACA number.

### Why rounded leading edge, sharp trailing edge

The leading edge must be **rounded**, not sharp. A sharp front edge only works at exactly 0° angle of attack — any slight deviation (from vibration, motor wake, turbulence) forces air to whip around a tight corner, causing immediate flow separation. A rounded edge gives air a gentle ramp to follow over a wide range of incoming angles (±5–10°), keeping flow attached.

The trailing edge is the opposite: it should be **thin/tapered**. This is where the two airstreams rejoin. A blunt trailing edge creates a wide wake with strong alternating vortex shedding (noise). A thin trailing edge lets the streams merge smoothly with a minimal wake.

This asymmetry — round in front, thin in back — is why the teardrop/airfoil shape is universal across wings, fan blades, and helicopter rotors.

## Practical takeaway for this project

- **Airfoil cross-section** on the 3D-printed blade dramatically reduces the tonal whine from vortex shedding.
- **Taper toward the tips** to reduce chord length where velocity is highest.
- **Round the tips in 3D** (not just a 2D radius in the plan-form) to avoid tip vortices.
- **Smooth surfaces** matter — sand or post-process if layer lines run perpendicular to airflow.
- **LED channel placement**: keep it shallow and on the trailing face, or fill with clear resin to restore the smooth profile. An open channel facing the airflow acts like a whistle.
