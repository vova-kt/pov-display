# Fisheye distortion

## What it is

A fisheye lens does not scale an image uniformly. It magnifies the center more than the edge, so straight radial distances are compressed nonlinearly as they approach the outer field of view. The visual result is a center bulge with an edge that feels comparatively anchored.

## Why it matters here

The POV display stores frames as polar slices and LED radii. A uniform radial zoom makes every radius move by the same factor, which can feel like the whole image is being pulled through the hub. A fisheye-style scale instead concentrates motion near the center while leaving the outer radius steadier. That makes text, images, and polar-native patterns read as a lens pulse rather than a simple zoom.

## Practical takeaway

Use fisheye scaling when the desired effect is a breathing center bulge without losing the outer silhouette. It should stay as a post-pattern framebuffer animation so every pattern gets the same lens behavior and the frame loops do not need pattern-specific branches.
