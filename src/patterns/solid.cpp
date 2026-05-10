#include "solid.h"

void SolidPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t) {
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            fb.setPixel(s, l, cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
        }
    }
    fb.swap();
}
