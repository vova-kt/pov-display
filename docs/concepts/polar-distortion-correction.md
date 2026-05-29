# Polar distortion correction

## The problem

The framebuffer is a 2D grid indexed by `(slice, led)` — angular position and radial distance. Patterns that draw rectangular content (text, images) treat this grid as Cartesian, but the display renders it in polar coordinates. Three distortions result:

**Angular stretch.** Arc length per slice grows linearly with radius: `arc = 2π × r / numSlices`. At 360 slices, the innermost LED traces a ~0.7 mm arc per slice while the outermost traces ~6.5 mm — a 10:1 ratio. Characters near the edge look horizontally stretched; near the hub, compressed.

**Curvature.** A horizontal line in framebuffer space (constant LED, varying slice) traces a circular arc on the disc, not a straight line. Curvature = 1/r, so lines near the hub curve severely.

**Non-uniform sampling density.** Inner rings represent tiny physical areas; outer rings span wide bands. Point-sampling a rectangular source image on this grid aliases at the center and wastes resolution at the edge.

## The virtual canvas approach

Instead of drawing directly in polar framebuffer coordinates, patterns render onto a rectangular **Canvas** (Cartesian x,y) and then an inverse polar mapping transforms the result into the framebuffer.

```
[Pattern] → draws to → [Canvas W×H]
                            ↓  inverse polar mapping
                      [Framebuffer slice×led]
```

The transform is a swappable strategy (`CoordTransform` interface) — patterns choose which transform they need. Polar-native patterns (solid, rainbow, scanner) skip both Canvas and transform entirely.

## Inverse polar mapping math

For each destination pixel `(slice, led)` in the polar framebuffer, compute the corresponding source `(x, y)` on the canvas:

```
θ       = slice × 2π / numSlices
r       = (led + 0.5) / numLeds           — normalized 0..1, +0.5 samples LED center
canvas_x = (r × cos(θ) + 1) × canvasW / 2
canvas_y = (1 − r × sin(θ)) × canvasH / 2
```

This is **inverse mapping** — iterating the destination, pulling from the source. Every destination pixel gets a value (no gaps), which is why all geometric transforms in image processing (OpenCV's `remap`, GPU texture mapping) use this direction.

## Pre-computed lookup table

Computing `cos` and `sin` per pixel per frame is wasteful. Instead, `PolarTransform::buildLut()` runs once at initialization (and again if dimensions change), storing each `(canvas_x, canvas_y)` pair as 8.8 fixed-point integers.

At render time, `apply()` loops through the LUT — one integer read, one canvas sample, one framebuffer write per pixel. No floating-point, no trig.

LUT memory for 360 slices × 40 LEDs × 4 bytes = ~57 KB. The Canvas itself at 80×80 pixels × 4 bytes = ~25 KB.

## Canvas sizing

The canvas should be `2 × numLeds` per side — matching radial resolution (numLeds pixels across the radius, 2× that across the diameter). For 40 LEDs this gives an 80×80 canvas.

At the outer edge, one canvas pixel spans roughly one LED pitch — 1:1 radial resolution. Inner LEDs heavily oversample the canvas (many polar pixels map to the same canvas pixel), which naturally provides anti-aliasing at the center where distortion would otherwise be worst.

## When to use polar correction vs. direct polar rendering

**Use the virtual canvas** for content authored in rectangular coordinates: text, bitmap images, UI elements, geometric shapes with straight edges.

**Render directly in polar space** for patterns that are naturally radial: solid fills, rainbow hue sweeps, radial scanners, spiral effects. These patterns are simpler and more efficient when expressed in (slice, led) coordinates — the polar geometry is a feature, not a distortion.

## Practical takeaway

- `PolarTransform` eliminates visible distortion for text and images with no per-frame trig cost
- Canvas + LUT add ~48 KB to memory usage — fits within ESP32-C6's budget alongside the double-buffered framebuffer
- The `CoordTransform` interface is extensible — future strategies (bilinear interpolation, log-polar, hub-offset correction) slot in without changing patterns or the framebuffer
