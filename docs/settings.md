# Settings system

## Why a registry

Both the MCU web UI and the WASM simulator need the same set of controls. The old approach duplicated every setting twice: hand-written HTML on each side, separate JSON serialization, separate validation, defaults pasted into five files. Adding one setting meant five edits that could drift independently.

The registry eliminates drift by making C++ the single source of truth. Both backends serialize from the same code; the shared JS renderer in `sim/js/settings_ui.js` builds the form from the resulting JSON model. Adding a setting is one array entry; the rest is automatic.

## Param semantics

Every configurable value — whether a top-level Config field, a pattern-specific param, or an animation param — is described by `Param` (`src/param.h`):

- **Int** — numeric with `min`/`max` range clamping. Optional `scale` field (e.g. `scale=10` means the stored int is mm × 10; the renderer displays `value / scale`).
- **Bool** — `0` / `1`; rendered as a checkbox.
- **Color** — packed 0xRRGGBB into `value`; rendered as a color picker.
- **Text** — backed by a fixed `char[]` buffer in the owning object; rendered as a text input.
- **Enum** — validated against a `ParamOption[]` list; rendered as a select.

Text params are stored as UTF-8 bytes rather than converted to a wide-character buffer. `TextPattern` keeps a decoded run cache and refreshes it only when the text bytes change, using a cheap bounded compare each frame because params are exposed as raw buffers. The compact bitmap tables live in `src/fonts/text_font*.h`, split by script so adding another simple alphabet does not touch pattern modes. The font layer owns UTF-8 iteration, run measurement, and glyph width metadata. Text animation modes slice that decoded run in `src/patterns/text.cpp`; word mode selects one non-space run per step instead of accumulating prefixes, so each word can be centered and scaled on its own. Text layout uses a configurable Cartesian margin measured in LED pitches; centered modes fit inside it with fixed-point scale steps, and marquee uses it as the scroll viewport.

Centered text has one deliberate low-scale escape hatch in `src/patterns/text.cpp`: short odd-length lines can be nudged sideways so the physical no-LED center gap falls between glyphs instead of through the middle glyph. This is a removable layout workaround for the current pixel count, not a new transform strategy; the polar sampling tradeoff is covered in `docs/concepts/polar-distortion-correction.md`.

## Scope filter

Settings carry a `Scope` tag: `Both`, `McuOnly`, or `SimOnly`. The registry's `toJson()` pre-filters by scope, so each client receives only what applies. `escPulseUs` is `McuOnly` (no motor in the sim). Sim diagnostics (jitter sliders, geometry mm, displayHz) are `SimOnly`.

For the wire format and the full registered entry list, see `src/settings_registry.cpp` directly — duplicating the schema here would rot.

## UI structure

The renderer produces two tabs:
- **Picture** (default) — brightness, color, pattern selector, mirror, radial balance, phase offset, animation params, and the active pattern's own params.
- **Hardware** — arm count, refresh rate, LED/slice counts, SPI clock, plus sim-only geometry and timing distortion controls.

Pattern param panels are all rendered but only the active pattern's panel is visible. Switching the Pattern selector updates visibility client-side without a re-fetch.

Animation params are shown with the picture controls because they alter the rendered image rather than the hardware timing. Rotation direction lives there for that reason: it reverses the phase animation without implying the motor or hall sensor wiring changed.
