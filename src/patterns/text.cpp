#include "text.h"
#include "../fonts/font5x7.h"
#include <cstring>
#include <pgmspace.h>

void TextPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t) {
    fb.clearBack();

    uint16_t textLen = strlen(cfg.text);
    if (textLen == 0) { fb.swap(); return; }

    uint16_t baseCols = textLen * (FONT_CHAR_WIDTH + 1) - 1;
    uint8_t scaleV = fb.numLeds() / FONT_CHAR_HEIGHT;
    uint8_t scaleH = fb.numSlices() / baseCols;
    uint8_t scale = scaleV < scaleH ? scaleV : scaleH;
    if (scale < 1) scale = 1;

    uint16_t charW    = FONT_CHAR_WIDTH * scale;
    uint16_t gap      = scale;
    uint16_t totalW   = textLen * (charW + gap) - gap;
    uint16_t totalH   = FONT_CHAR_HEIGHT * scale;

    uint16_t startSlice = 0;
    if (totalW < fb.numSlices())
        startSlice = (fb.numSlices() - totalW) / 2;

    uint16_t ledOffset = 0;
    if (totalH < fb.numLeds())
        ledOffset = (fb.numLeds() - totalH) / 2;

    uint16_t slicePos = startSlice;
    for (uint16_t ch = 0; ch < textLen && slicePos < fb.numSlices(); ch++) {
        uint8_t c = (uint8_t)cfg.text[ch];
        if (c < FONT_FIRST_CHAR || c > 126) c = FONT_FIRST_CHAR;
        uint8_t ci = c - FONT_FIRST_CHAR;

        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++) {
            uint8_t colData = pgm_read_byte(&FONT_5X7[ci][col]);
            for (uint8_t row = 0; row < FONT_CHAR_HEIGHT; row++) {
                if (colData & (1 << row)) {
                    for (uint8_t dy = 0; dy < scale; dy++) {
                        uint16_t led = ledOffset + row * scale + dy;
                        if (led >= fb.numLeds()) break;
                        for (uint8_t dx = 0; dx < scale; dx++) {
                            uint16_t s = slicePos + col * scale + dx;
                            if (s < fb.numSlices())
                                fb.setPixel(s, led, cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
                        }
                    }
                }
            }
        }
        slicePos += charW + gap;
    }
    fb.swap();
}
