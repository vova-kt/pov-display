#pragma once
#include "../coord_transform.h"
#include "../canvas.h"
#include "../framebuffer.h"
#include "../config.h"

class IdentityTransform : public CoordTransform {
public:
    void apply(const Canvas& canvas, Framebuffer& fb, const Config&) override {
        uint16_t w = canvas.width()  < fb.numSlices() ? canvas.width()  : fb.numSlices();
        uint16_t h = canvas.height() < fb.numLeds()   ? canvas.height() : fb.numLeds();
        for (uint16_t x = 0; x < w; x++) {
            for (uint16_t y = 0; y < h; y++) {
                Pixel p = canvas.pixelAt(x, y);
                if (p.brightness == 0) continue;
                fb.setPixel(x, y, p.red, p.green, p.blue, p.brightness & 0x1F);
            }
        }
    }
};
