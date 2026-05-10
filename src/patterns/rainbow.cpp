#include "rainbow.h"

void RainbowPattern::hsvToRgb(uint16_t h, uint8_t s, uint8_t v,
                               uint8_t& r, uint8_t& g, uint8_t& b) {
    // Integer-only HSV→RGB. h: 0..359, s: 0..255, v: 0..255
    if (s == 0) { r = g = b = v; return; }

    uint8_t region  = h / 60;
    uint8_t rem     = (h % 60) * 255 / 60;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * rem) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - rem)) >> 8))) >> 8;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

void RainbowPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) {
    uint16_t hueShift = (timeMs / 20) % 360;  // slow rotation

    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        uint16_t hue = (s * 360 / fb.numSlices() + hueShift) % 360;
        uint8_t r, g, b;
        hsvToRgb(hue, 255, 255, r, g, b);

        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            fb.setPixel(s, l, r, g, b, cfg.brightness);
        }
    }
    fb.swap();
}
