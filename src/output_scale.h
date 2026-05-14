#pragma once
#include <cstdint>
#include <cstring>

static constexpr float MAX_RADIAL_BOOST = 3.0f;

// Compute per-LED output scale. Returns true if any value < 255 (scaling needed).
inline bool compute_output_scale(uint8_t* out, uint16_t maxLeds,
                                 uint16_t numLeds, bool radialBalance) {
    memset(out, 255, maxLeds);

    if (radialBalance && numLeds > 1) {
        float floor = 1.0f / MAX_RADIAL_BOOST;
        float rMax = numLeds - 0.5f;
        for (uint16_t i = 0; i < numLeds && i < maxLeds; i++) {
            float rNorm = (i + 0.5f) / rMax;
            if (rNorm < floor) rNorm = floor;
            out[i] = (uint8_t)(rNorm * 255.0f + 0.5f);
        }
    }

    for (uint16_t i = 0; i < maxLeds; i++) {
        if (out[i] < 255) return true;
    }
    return false;
}
