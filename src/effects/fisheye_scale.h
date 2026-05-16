#pragma once
#include "../effect.h"
#include "../framebuffer.h"

namespace {
constexpr ParamOption kFisheyeScaleDurationOptions[] = {
    {"200 ms", 200}, {"500 ms", 500}, {"1000 ms", 1000}, {"2000 ms", 2000}
};

constexpr ParamOption kFisheyeScaleFactorOptions[] = {
    {"1.2", 12}, {"1.5", 15}, {"2.0", 20}, {"3.0", 30}
};
}

class FisheyeScaleEffect : public Effect {
public:
    FisheyeScaleEffect() {
        params_ = storage_;
        paramCount_ = 2;
    }

    const char* name() const override { return "Fisheye Scale"; }
    const char* key() const override { return "fisheye"; }
    bool active() const override { return true; }

    void apply(EffectState&, Framebuffer& fb, uint32_t timeMs) override {
        int32_t durationMs = storage_[0].value;
        int32_t maxScaleTenths = storage_[1].value;
        if (durationMs <= 0 || maxScaleTenths <= 10) return;

        int32_t halfMs = durationMs / 2;
        if (halfMs <= 0) return;

        int32_t cycleMs = (int32_t)(timeMs % (uint32_t)durationMs);
        int32_t rampMs = cycleMs <= halfMs ? cycleMs : durationMs - cycleMs;
        int32_t centerShrinkQ8 = 256 - ((10 * 256 + maxScaleTenths / 2) / maxScaleTenths);
        int32_t effectQ8 = (centerShrinkQ8 * rampMs) / halfMs;

        if (effectQ8 > 0) applyFisheyeScale(fb, effectQ8);
    }

private:
    static uint16_t sourceLedFor(uint16_t led, uint16_t numLeds, int32_t effectQ8) {
        if (numLeds <= 1 || effectQ8 <= 0) return led;

        uint16_t maxLed = (uint16_t)(numLeds - 1);
        int32_t rQ8 = ((int32_t)led * 256 + maxLed / 2) / maxLed;
        if (rQ8 > 256) rQ8 = 256;

        int32_t invQ8 = 256 - rQ8;
        int32_t invSqQ8 = (invQ8 * invQ8 + 128) >> 8;
        int32_t shrinkQ8 = (effectQ8 * invSqQ8 + 128) >> 8;
        int32_t ratioQ8 = 256 - shrinkQ8;
        if (ratioQ8 < 0) ratioQ8 = 0;

        uint16_t srcLed = (uint16_t)(((int32_t)led * ratioQ8 + 128) >> 8);
        return srcLed < numLeds ? srcLed : maxLed;
    }

    static void applyFisheyeScale(Framebuffer& fb, int32_t effectQ8) {
        uint16_t numSlices = fb.numSlices();
        uint16_t numLeds = fb.numLeds();
        if (numLeds == 0) return;

        for (int32_t led = (int32_t)numLeds - 1; led >= 0; led--) {
            uint16_t srcLed = sourceLedFor((uint16_t)led, numLeds, effectQ8);
            for (uint16_t s = 0; s < numSlices; s++) {
                Pixel* slice = fb.backSlice(s);
                slice[led] = slice[srcLed];
            }
        }
    }

    Param storage_[2] = {{
        "duration", "Duration", ParamType::Enum,
        500, 500, 200, 2000,
        kFisheyeScaleDurationOptions, 4,
        nullptr, 0
    }, {
        "factor", "Scale factor", ParamType::Enum,
        20, 20, 12, 30,
        kFisheyeScaleFactorOptions, 4,
        nullptr, 0
    }};
};
