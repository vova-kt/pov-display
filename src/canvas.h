#pragma once
#include <cstdint>
#include "framebuffer.h"  // Pixel struct

class Canvas {
public:
    bool init(uint16_t width, uint16_t height);
    void release();
    void clear();

    void setPixel(uint16_t x, uint16_t y,
                  uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    Pixel pixelAt(uint16_t x, uint16_t y) const;

    uint16_t width()  const { return width_; }
    uint16_t height() const { return height_; }

private:
    Pixel* buffer_   = nullptr;
    uint16_t width_  = 0;
    uint16_t height_ = 0;
};
