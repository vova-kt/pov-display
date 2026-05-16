#pragma once
#include "../effect.h"
#include "../framebuffer.h"
#include "../config.h"
#include <cstring>

namespace {
constexpr ParamOption kBloomRadiusOptions[] = {
    {"1 LED", 1}, {"2 LEDs", 2}, {"3 LEDs", 3}, {"4 LEDs", 4}
};

constexpr ParamOption kBloomIntensityOptions[] = {
    {"25%", 64}, {"50%", 128}, {"75%", 192}, {"100%", 255}
};

constexpr ParamOption kBloomThresholdOptions[] = {
    {"Off", 0}, {"Low", 64}, {"Medium", 128}, {"High", 192}
};
}

class BloomEffect : public Effect {
public:
    BloomEffect() {
        params_ = storage_;
        paramCount_ = 3;
    }

    const char* name() const override { return "Bloom"; }
    const char* key() const override { return "bloom"; }
    bool active() const override { return true; }

    void apply(EffectState&, Framebuffer& fb, uint32_t) override {
        int32_t radius = storage_[0].value;
        int32_t intensityQ8 = storage_[1].value;
        int32_t threshold = storage_[2].value;

        if (radius <= 0 || intensityQ8 <= 0) return;

        uint16_t numSlices = fb.numSlices();
        uint16_t numLeds = fb.numLeds();
        if (numLeds == 0) return;

        Pixel scratch[MAX_LEDS];

        for (uint16_t s = 0; s < numSlices; s++) {
            Pixel* slice = fb.backSlice(s);
            memcpy(scratch, slice, numLeds * sizeof(Pixel));

            for (uint16_t led = 0; led < numLeds; led++) {
                int32_t bloomR = 0, bloomG = 0, bloomB = 0;
                int32_t totalWeight = 0;

                int32_t lo = (int32_t)led - radius;
                int32_t hi = (int32_t)led + radius;
                if (lo < 0) lo = 0;
                if (hi >= (int32_t)numLeds) hi = (int32_t)numLeds - 1;

                for (int32_t n = lo; n <= hi; n++) {
                    if (n == (int32_t)led) continue;

                    const Pixel& p = scratch[n];
                    if (p.red < threshold && p.green < threshold && p.blue < threshold)
                        continue;

                    int32_t dist = (int32_t)led - n;
                    if (dist < 0) dist = -dist;
                    int32_t weight = radius + 1 - dist;

                    bloomR += p.red * weight;
                    bloomG += p.green * weight;
                    bloomB += p.blue * weight;
                    totalWeight += weight;
                }

                if (totalWeight > 0) {
                    bloomR /= totalWeight;
                    bloomG /= totalWeight;
                    bloomB /= totalWeight;

                    int32_t addR = (bloomR * intensityQ8 + 128) >> 8;
                    int32_t addG = (bloomG * intensityQ8 + 128) >> 8;
                    int32_t addB = (bloomB * intensityQ8 + 128) >> 8;

                    int32_t r = slice[led].red   + addR;
                    int32_t g = slice[led].green + addG;
                    int32_t b = slice[led].blue  + addB;

                    slice[led].red   = r > 255 ? 255 : (uint8_t)r;
                    slice[led].green = g > 255 ? 255 : (uint8_t)g;
                    slice[led].blue  = b > 255 ? 255 : (uint8_t)b;

                    if (addR | addG | addB) {
                        uint8_t srcBr = maxBrightness(scratch, lo, hi, (int32_t)led);
                        if (srcBr > (slice[led].brightness & 0x1F))
                            slice[led].brightness = 0xE0 | srcBr;
                    }
                }
            }
        }
    }

private:
    static uint8_t maxBrightness(const Pixel* buf, int32_t lo, int32_t hi, int32_t self) {
        uint8_t best = 0;
        for (int32_t i = lo; i <= hi; i++) {
            if (i == self) continue;
            uint8_t br = buf[i].brightness & 0x1F;
            if (br > best) best = br;
        }
        return best;
    }

    Param storage_[3] = {{
        "radius", "Radius", ParamType::Enum,
        2, 2, 1, 4,
        kBloomRadiusOptions, 4,
        nullptr, 0
    }, {
        "intensity", "Intensity", ParamType::Enum,
        128, 128, 64, 255,
        kBloomIntensityOptions, 4,
        nullptr, 0
    }, {
        "threshold", "Threshold", ParamType::Enum,
        128, 128, 0, 192,
        kBloomThresholdOptions, 4,
        nullptr, 0
    }};
};
