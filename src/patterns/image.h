#pragma once
#include "pattern.h"
#include "../canvas.h"
#include "../transforms/polar_transform.h"

class ImagePattern : public Pattern {
public:
    ~ImagePattern() override { canvas_.release(); }
    void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) override;
    const char* name() const override { return "image"; }

    bool loadImage(const uint8_t* rgbData, uint16_t width, uint16_t height);
    bool hasImage() const { return loaded_; }

private:
    Canvas canvas_;
    PolarTransform transform_;
    uint16_t lastSlices_ = 0;
    uint16_t lastLeds_   = 0;
    bool loaded_ = false;
};
