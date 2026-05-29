# Dual-MCU wireless testing setup

This is an archived testing design. The source files are still kept in `src/`, but `stationary` and `rotating` are not active PlatformIO environments; the only active firmware target is `esp32c6`.

How to test the POV display hardware before the slip ring arrives, using two XIAO ESP32 boards communicating over WiFi. The stationary side runs a C6 (the production board); the rotating side can be any XIAO ESP32 variant — C6 or C3. Both are single-core RISC-V with WiFi and SPI, which is all the rotating side needs.

## Physical setup

```
                            ┌─ hollow shaft ─┐
                            │                │
  ┌──────────────────┐      │   ┌──────────────────────────┐
  │ Stationary MCU   │      │   │ Rotating arm             │
  │                  │ WiFi │   │                          │
  │  Hall sensor ────┤ ~~~~ │   │  MCU ─── SPI ─── LEDs   │
  │  ESC / motor ────┤      │   │   │                      │
  │  WiFi AP + WebUI │      │   │  LiPo (500–1000 mAh)    │
  │                  │      │   │                          │
  │  3S / PD power   │      │   └──────────────────────────┘
  └──────────────────┘      │
                            └────────────────┘
```

**Stationary side** — identical to the current single-MCU firmware plus a UDP timing broadcaster. Hall sensor, motor ESC, WiFi AP, and web UI all stay where they are.

**Rotating side** — a second XIAO ESP32-C6 mounted on the arm, battery-powered, running patterns + LED output. Connects to the stationary AP as a WiFi client and receives rotation timing over UDP.

No wires cross the rotation boundary. The slip ring is bypassed entirely.

## Why two MCUs instead of one on the arm

Putting everything on the rotating side would require either wireless hall-sensor forwarding from the stationary side (same latency problem) or mounting the hall sensor on the arm (mechanically awkward — the magnet is on the stationary frame). Keeping the stationary MCU unchanged means the existing firmware, web UI, and motor control work unmodified. The rotating MCU only needs timing data and config.

## Power budget — rotating side

The rotating MCU runs from a small LiPo (no buck converter, no motor load). Power path: single-cell LiPo (3.7 V nominal) → XIAO's onboard charger/regulator → 3.3 V MCU + 5 V pin for LEDs.

### Consumers

| Component | Current at 5 V | Notes |
|-----------|---------------|-------|
| ESP32-C6 (WiFi active) | ~130 mA | Through onboard LDO |
| 26 LEDs, brightness 8 (of 31) | ~200 mA | ~25% duty, mixed colors |
| 26 LEDs, brightness 4 | ~100 mA | Conservative cap for longer runtime |

### Runtime estimates

Through the XIAO's onboard boost/LDO (~85% efficiency from LiPo to 5 V rail):

| Battery | Brightness cap | Total draw from LiPo | Runtime |
|---------|---------------|----------------------|---------|
| 500 mAh | 4 | ~320 mA | ~1.5 hr |
| 500 mAh | 8 | ~450 mA | ~1.1 hr |
| 1000 mAh (2×500) | 4 | ~320 mA | ~3 hr |
| 1000 mAh (2×500) | 8 | ~450 mA | ~2.2 hr |

**Recommendation:** cap `maxBrightness` to 8 for the rotating build. Enough for indoor testing, and a single 500 mAh cell gives over an hour.

## Communication protocol

Two data streams flow from stationary → rotating:

### 1. Rotation timing — UDP, fire-and-forget

On every hall sensor trigger, the stationary MCU sends a small UDP packet to the rotating MCU's IP (or broadcast to `192.168.4.255`):

```
┌──────────────────────────────┐
│ uint32_t  rotationPeriodUs   │  4 bytes
│ uint32_t  timestampMs        │  4 bytes
└──────────────────────────────┘
```

8 bytes, ~30–60 packets/sec at typical RPM. Lost packets are fine — the next one comes within 16–33 ms and carries fresh timing. UDP latency on a local AP with one client is typically < 2 ms, well within the timing tolerance (one slice at 360 slices / 60 Hz = ~46 µs, but timing only needs to be accurate per-revolution, not per-slice — the local `esp_timer` free-runs between hall events).

### 2. Config changes — UDP broadcast

When the web UI changes a setting, the stationary MCU broadcasts the raw JSON patch (the POST body) via UDP on port 4211. Config patches are small (typically < 200 bytes), well within a single UDP packet.

The rotating MCU receives the patch and calls `settings_registry::applyJson()` — same code path the web server uses. If a packet is lost, the user clicks the slider again; for a testing setup this is fine.

### Why not the other direction?

The rotating MCU never needs to send data back. Status (RPM, heap) comes from the stationary side's hall sensor and web server. If the rotating MCU needs status reporting later, a periodic UDP heartbeat back is trivial to add.

## Software architecture

### TimingSource abstraction

The only tight coupling between hall sensor and rendering is in `SliceScheduler::onNewRotation()`, which reads `hall_->rotationPeriodUs()`. Abstract this behind an interface:

```cpp
// src/timing_source.h
class TimingSource {
public:
    virtual ~TimingSource() = default;
    virtual bool    consumeNewRotation() = 0;
    virtual uint32_t rotationPeriodUs() const = 0;
    virtual uint32_t lastTriggerMs()    const = 0;
};
```

Two implementations:
- **`HallTimingSource`** — wraps `HallSensor`. Used in single-MCU and stationary builds. Trivial adapter, just delegates.
- **`WifiTimingSource`** — listens for UDP packets, updates period and trigger timestamp. Used in the rotating build.

`SliceScheduler` and `main.cpp`'s spinning detection both switch from `HallSensor*` to `TimingSource*`. No other code knows or cares where timing comes from.

### Build modes

The code was structured around three entry points, each with its own `main_*.cpp` and `build_src_filter` when the wireless setup was active:

| Environment | Board | Entry point | What it includes |
|-------------|-------|-------------|-----------------|
| `esp32c6` | C6 | `main.cpp` | Everything (current single-MCU) |
| `stationary` | C6 | `main_stationary.cpp` | Hall + motor + WiFi AP + web server + timing TX + config relay |
| `rotating` | C3 | `main_rotating.cpp` | WiFi client + timing RX + config RX + patterns + LEDs |

To use this setup again, reintroduce the `stationary` and `rotating` environments in `platformio.ini`. To use a C6 as the rotating board instead of C3, set that environment's board to `seeed_xiao_esp32c6`.

Each entry point wires up the appropriate `TimingSource` and peripherals. No `#ifdef` soup in shared code — the `TimingSource` abstraction is the only seam.

The stationary build includes all pattern/effect source files so that `settings_registry::toJson()` links correctly and the web UI shows real pattern names. These are dead code at runtime but ensure the web UI works identically to the single-MCU build.

### Pin assignments

All four pin assignments in `src/config.h` use `#ifndef` guards, matching the existing pattern for `MAX_LEDS`. Override via `build_flags` if the C3's pin labels need different values:

```ini
build_flags = -DPIN_LED_CLK=D8 -DPIN_LED_MOSI=D10
```

### Config relay

The web server has a `RelayCallback` that fires with the raw POST body before the body buffer is cleared. The stationary build registers this callback to UDP-broadcast the JSON patch to the rotating MCU on port 4211. The rotating build listens on the same port and feeds received patches into `settings_registry::applyJson()`.

## Build & flash

The archived wireless targets do not build until their PlatformIO environments are restored. The active single-MCU firmware still uses `pio run -e esp32c6 -t upload`.

## Timing accuracy

**Will WiFi latency ruin the image?**

No. The hall sensor fires once per revolution. Between hall events, the local `esp_timer` on the rendering MCU free-runs at the calculated slice interval. WiFi latency (1–5 ms typical on a quiet local AP) only affects how quickly the rotating MCU learns about a *change* in RPM — it doesn't accumulate per-slice.

At 30 Hz (1800 RPM with 2 arms), one revolution is 33 ms. A 3 ms WiFi delay means the rotating MCU's timer restarts 3 ms late — that's ~9% of a revolution, or ~32 slices of phase offset at 360 slices. This is a fixed offset (not jitter), and the phase control already exists in the UI.

At steady RPM (motor under PID or constant ESC pulse), the offset is stable and the image is coherent. RPM transients during spinup will cause brief smearing — acceptable for testing.

## Tradeoffs vs slip ring

| | Slip ring (single MCU) | Dual MCU wireless |
|---|---|---|
| Latency | Zero — hall ISR drives timer directly | 1–5 ms per revolution (stable offset) |
| Reliability | Wired — deterministic | WiFi can drop packets; mitigated by fire-and-forget design |
| Power | Unlimited (mains or large battery on stationary side) | Limited by onboard LiPo (~1–3 hr) |
| Complexity | One firmware, one MCU | Two firmwares, WiFi transport layer |
| Config control | Single web UI, direct | Web UI on stationary, relayed to rotating |
| Hardware needed | Slip ring + wiring through shaft | Second XIAO (C3 or C6) + LiPo + nothing else |

The wireless setup is a testing bridge, not the final architecture. Once the slip ring arrives, switch back to `pio run -e esp32c6` and everything is as before.
