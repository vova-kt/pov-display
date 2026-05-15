#include "image.h"
#include <cstring>

bool ImagePattern::loadImage(const uint8_t* rgbData, uint16_t width, uint16_t height) {
    canvas_.release();
    if (!canvas_.init(width, height)) return false;

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            size_t i = ((size_t)y * width + x) * 3;
            canvas_.setPixel(x, y, rgbData[i], rgbData[i + 1], rgbData[i + 2], 31);
        }
    }

    lastSlices_ = 0;
    loaded_ = true;
    return true;
}

void ImagePattern::generate(Framebuffer& fb, const Config& cfg, uint32_t) {
    fb.clearBack();

    if (!loaded_) { fb.swap(); return; }

    uint16_t ns = fb.numSlices();
    uint16_t nl = fb.numLeds();
    if (ns != lastSlices_ || nl != lastLeds_) {
        transform_.buildLut(ns, nl, canvas_.width(), canvas_.height());
        lastSlices_ = ns;
        lastLeds_   = nl;
    }

    transform_.apply(canvas_, fb, cfg);
    fb.swap();
}
