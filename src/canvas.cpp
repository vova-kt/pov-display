#include "canvas.h"
#include <cstdlib>
#include <cstring>

bool Canvas::init(uint16_t width, uint16_t height) {
    release();
    width_  = width;
    height_ = height;
    size_t sz = (size_t)width * height * sizeof(Pixel);
    buffer_ = (Pixel*)malloc(sz);
    if (!buffer_) {
        width_ = height_ = 0;
        return false;
    }
    memset(buffer_, 0, sz);
    return true;
}

void Canvas::release() {
    free(buffer_);
    buffer_ = nullptr;
    width_ = height_ = 0;
}

void Canvas::clear() {
    if (buffer_)
        memset(buffer_, 0, (size_t)width_ * height_ * sizeof(Pixel));
}

void Canvas::setPixel(uint16_t x, uint16_t y,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    if (x >= width_ || y >= height_) return;
    Pixel& p = buffer_[(size_t)y * width_ + x];
    p.brightness = kHd107sBrightnessPrefix | (brightness & kHd107sBrightnessMask);
    p.blue  = b;
    p.green = g;
    p.red   = r;
}

Pixel Canvas::pixelAt(uint16_t x, uint16_t y) const {
    if (!buffer_ || x >= width_ || y >= height_)
        return Pixel{0, 0, 0, 0};
    return buffer_[(size_t)y * width_ + x];
}
