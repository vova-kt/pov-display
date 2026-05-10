#include "text.h"
#include "../fonts/font5x7.h"
#include <cstring>
#include <pgmspace.h>

void TextPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t) {
    fb.clearBack();

    uint16_t textLen = strlen(cfg.text);
    if (textLen == 0) { fb.swap(); return; }

    // Each char: 5 columns + 1 gap = 6 slices
    uint16_t totalCols = textLen * (FONT_CHAR_WIDTH + 1) - 1;
    // Center the text in the available slices
    uint16_t startSlice = 0;
    if (totalCols < fb.numSlices())
        startSlice = (fb.numSlices() - totalCols) / 2;

    // Vertical centering: font is 7px tall, place in middle of LED strip
    uint16_t ledOffset = 0;
    if (fb.numLeds() > FONT_CHAR_HEIGHT)
        ledOffset = (fb.numLeds() - FONT_CHAR_HEIGHT) / 2;

    uint16_t slicePos = startSlice;
    for (uint16_t ch = 0; ch < textLen && slicePos < fb.numSlices(); ch++) {
        uint8_t c = (uint8_t)cfg.text[ch];
        if (c < FONT_FIRST_CHAR || c > 126) c = FONT_FIRST_CHAR; // replace unknown with space
        uint8_t ci = c - FONT_FIRST_CHAR;

        for (uint8_t col = 0; col < FONT_CHAR_WIDTH && slicePos < fb.numSlices(); col++) {
            uint8_t colData = pgm_read_byte(&FONT_5X7[ci][col]);
            for (uint8_t row = 0; row < FONT_CHAR_HEIGHT; row++) {
                if (colData & (1 << row)) {
                    uint16_t led = ledOffset + row;
                    if (led < fb.numLeds()) {
                        fb.setPixel(slicePos, led, cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
                    }
                }
            }
            slicePos++;
        }
        slicePos++; // inter-character gap
    }
    fb.swap();
}
