#pragma once
#include <cstdint>
#include <cstddef>

static constexpr uint8_t kHd107sBrightnessPrefix = 0xE0; // HD107S top-3-bit marker (111xxxxx)
static constexpr uint8_t kHd107sBrightnessMask   = 0x1F; // 5-bit brightness value field

struct __attribute__((packed)) Pixel {
    uint8_t brightness; // kHd107sBrightnessPrefix | (0..31)
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

static constexpr Pixel kBlackPixel = {kHd107sBrightnessPrefix, 0, 0, 0};

class Framebuffer {
public:
    bool init(uint16_t numSlices, uint16_t numLeds);
    void release();
    bool resize(uint16_t numSlices, uint16_t numLeds);

    void setPixel(uint16_t slice, uint16_t led, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void clearBack();

    const Pixel* getSlice(uint16_t sliceIndex) const;
    Pixel* backSlice(uint16_t sliceIndex);
    Pixel* backBuffer();
    void swap();

    uint16_t numSlices() const { return numSlices_; }
    uint16_t numLeds()   const { return numLeds_; }

private:
    Pixel* buffers_[2]    = {nullptr, nullptr};
    volatile uint8_t front_ = 0;
    uint16_t numSlices_   = 0;
    uint16_t numLeds_     = 0;

    size_t bufferSize() const;
};
