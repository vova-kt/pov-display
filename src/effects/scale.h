#pragma once
#include "../effect.h"
#include "../framebuffer.h"

namespace {
constexpr ParamOption kScaleDurationOptions[] = {
    {"200 ms", 200}, {"500 ms", 500}, {"1000 ms", 1000}, {"2000 ms", 2000}
};

constexpr ParamOption kScaleFactorOptions[] = {
    {"1.2", 12}, {"1.5", 15}, {"2.0", 20}, {"3.0", 30}
};
}

class ScaleEffect : public Effect {
public:
    ScaleEffect() {
        params_ = storage_;
        paramCount_ = 2;
    }

    const char* name() const override { return "Scale"; }
    const char* key() const override { return "scale"; }
    bool active() const override { return true; }

    void apply(EffectState&, Framebuffer& fb, uint32_t timeMs) override {
        int32_t durationMs = storage_[0].value;
        int32_t maxScaleTenths = storage_[1].value;
        if (durationMs <= 0 || maxScaleTenths <= 10) return;

        int32_t halfMs = durationMs / 2;
        if (halfMs <= 0) return;

        int32_t cycleMs = (int32_t)(timeMs % (uint32_t)durationMs);
        int32_t amplitude = maxScaleTenths - 10;
        int32_t offset = cycleMs <= halfMs
            ? (amplitude * cycleMs) / halfMs
            : (amplitude * (durationMs - cycleMs)) / halfMs;
        int32_t scaleTenths = 10 + offset;

        if (scaleTenths > 10) applyRadialScale(fb, scaleTenths);
    }

private:
    static uint16_t sourceLedFor(uint16_t led, uint16_t numLeds, int32_t scaleTenths) {
        int32_t srcQ8 = ((((int32_t)led << 8) + 128) * 10) / scaleTenths - 128;
        if (srcQ8 < 0) return 0;

        uint16_t srcLed = (uint16_t)((srcQ8 + 128) >> 8);
        return srcLed < numLeds ? srcLed : (uint16_t)(numLeds - 1);
    }

    static void applyRadialScale(Framebuffer& fb, int32_t scaleTenths) {
        uint16_t numSlices = fb.numSlices();
        uint16_t numLeds = fb.numLeds();
        if (numLeds == 0) return;

        for (uint16_t s = 0; s < numSlices; s++) {
            Pixel* slice = fb.backSlice(s);
            for (int32_t led = (int32_t)numLeds - 1; led >= 0; led--) {
                uint16_t srcLed = sourceLedFor((uint16_t)led, numLeds, scaleTenths);
                slice[led] = slice[srcLed];
            }
        }
    }

    Param storage_[2] = {{
        "duration", "Duration", ParamType::Enum,
        500, 500, 200, 2000,
        kScaleDurationOptions, 4,
        nullptr, 0
    }, {
        "factor", "Scale factor", ParamType::Enum,
        15, 15, 12, 30,
        kScaleFactorOptions, 4,
        nullptr, 0
    }};
};
