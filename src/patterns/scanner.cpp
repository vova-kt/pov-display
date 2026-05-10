#include "scanner.h"
#include <Arduino.h>

void ScannerPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) {
    fb.clearBack();

    uint16_t n = fb.numLeds();
    uint16_t delay = 32;
    uint16_t pos = (timeMs / delay) % n;

    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            fb.setPixel(s, l, 0, 0, 0, cfg.brightness);
        }
    }
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            fb.setPixel(s, l, 0, 0, 0, cfg.brightness);
        }
        fb.setPixel(s, pos, cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    }
    fb.swap();

    static uint32_t lastLog = 0;
    if (timeMs - lastLog >= 10000) {
        lastLog = timeMs;
        Serial.printf("[scanner] pos=%u/%u delay=%ums rgb=(%u,%u,%u) br=%u slices=%u\n",
                      pos, n, delay, cfg.colorR, cfg.colorG, cfg.colorB,
                      cfg.brightness, fb.numSlices());
    }
}
