#include "polar_transform.h"
#include "../canvas.h"
#include "../framebuffer.h"
#include "../config.h"
#include <cmath>
#include <cstdlib>

static constexpr float TAU = 6.283185307179586f;

void PolarTransform::freeLut() {
    free(lut_);
    lut_ = nullptr;
    lutSlices_ = lutLeds_ = lutCanvasW_ = lutCanvasH_ = 0;
}

void PolarTransform::buildLut(uint16_t numSlices, uint16_t numLeds,
                               uint16_t canvasW, uint16_t canvasH) {
    if (lut_ && lutSlices_ == numSlices && lutLeds_ == numLeds &&
        lutCanvasW_ == canvasW && lutCanvasH_ == canvasH)
        return;

    freeLut();

    size_t count = (size_t)numSlices * numLeds;
    lut_ = (LutEntry*)malloc(count * sizeof(LutEntry));
    if (!lut_) return;

    lutSlices_  = numSlices;
    lutLeds_    = numLeds;
    lutCanvasW_ = canvasW;
    lutCanvasH_ = canvasH;

    for (uint16_t s = 0; s < numSlices; s++) {
        float angle = (float)s * TAU / (float)numSlices;
        float cosA  = cosf(angle);
        float sinA  = sinf(angle);

        for (uint16_t led = 0; led < numLeds; led++) {
            float radius = ((float)led + 0.5f) / (float)numLeds;

            float nx = radius * cosA;
            float ny = radius * sinA;

            float cx = (nx + 1.0f) * 0.5f * (float)canvasW;
            float cy = (1.0f - ny) * 0.5f * (float)canvasH;

            if (cx < 0.0f) cx = 0.0f;
            if (cy < 0.0f) cy = 0.0f;
            if (cx >= (float)canvasW) cx = (float)canvasW - 0.001f;
            if (cy >= (float)canvasH) cy = (float)canvasH - 0.001f;

            size_t idx = (size_t)s * numLeds + led;
            lut_[idx].cx = (uint16_t)(cx * 256.0f);
            lut_[idx].cy = (uint16_t)(cy * 256.0f);
        }
    }
}

void PolarTransform::apply(const Canvas& canvas, Framebuffer& fb, const Config&) {
    if (!lut_) return;

    uint16_t numSlices = fb.numSlices();
    uint16_t numLeds   = fb.numLeds();

    for (uint16_t s = 0; s < numSlices; s++) {
        for (uint16_t led = 0; led < numLeds; led++) {
            size_t idx = (size_t)s * numLeds + led;
            uint16_t cx = lut_[idx].cx >> 8;
            uint16_t cy = lut_[idx].cy >> 8;

            Pixel p = canvas.pixelAt(cx, cy);
            if (p.brightness == 0) continue;
            fb.setPixel(s, led, p.red, p.green, p.blue, p.brightness & kHd107sBrightnessMask);
        }
    }
}
