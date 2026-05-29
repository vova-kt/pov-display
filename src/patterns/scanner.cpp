#include "scanner.h"
#include "../log_tags.h"

LOG_TAG(scanner);

void ScannerPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) {
    fb.clearBack();

    uint16_t n = fb.numLeds();
    uint16_t delay = 32;
    uint16_t pos = (timeMs / delay) % n;

    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        fb.setPixel(s, pos, cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    }

    static uint32_t lastLog = 0;
    if (timeMs - lastLog >= 10000) {
        lastLog = timeMs;
        POV_LOGD("pos=%u/%u delay=%ums rgb=(%u,%u,%u) br=%u slices=%u",
                 pos, n, delay, cfg.colorR, cfg.colorG, cfg.colorB,
                 cfg.brightness, fb.numSlices());
    }
}
