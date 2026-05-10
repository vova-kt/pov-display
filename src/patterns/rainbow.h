#pragma once
#include "pattern.h"

class RainbowPattern : public Pattern {
public:
    void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) override;
    const char* name() const override { return "rainbow"; }

private:
    static void hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b);
};
