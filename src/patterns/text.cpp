#include "text.h"
#include "../fonts/font5x7.h"
#include <cstring>
#include <pgmspace.h>

void TextPattern::ensureCanvas(uint16_t numSlices, uint16_t numLeds) {
    uint16_t cw = numLeds * 2;
    uint16_t ch = numLeds * 2;

    if (cw == lastCanvasW_ && ch == lastCanvasH_ &&
        numSlices == lastSlices_ && numLeds == lastLeds_)
        return;

    canvas_.release();
    canvas_.init(cw, ch);
    transform_.buildLut(numSlices, numLeds, cw, ch);
    lastCanvasW_ = cw;
    lastCanvasH_ = ch;
    lastSlices_  = numSlices;
    lastLeds_    = numLeds;
}

void TextPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t) {
    fb.clearBack();

    uint16_t textLen = strlen(cfg.text);
    if (textLen == 0) { fb.swap(); return; }

    ensureCanvas(fb.numSlices(), fb.numLeds());
    canvas_.clear();

    uint16_t cw = canvas_.width();
    uint16_t ch = canvas_.height();

    uint16_t baseCols = textLen * (FONT_CHAR_WIDTH + 1) - 1;
    uint8_t scaleV = ch / FONT_CHAR_HEIGHT;
    uint8_t scaleH = cw / baseCols;
    uint8_t scale = scaleV < scaleH ? scaleV : scaleH;
    if (scale < 1) scale = 1;

    uint16_t charW  = FONT_CHAR_WIDTH * scale;
    uint16_t gap    = scale;
    uint16_t totalW = textLen * (charW + gap) - gap;
    uint16_t totalH = FONT_CHAR_HEIGHT * scale;

    uint16_t startX = 0;
    if (totalW < cw)
        startX = (cw - totalW) / 2;

    uint16_t startY = 0;
    if (totalH < ch)
        startY = (ch - totalH) / 2;

    uint16_t xPos = startX;
    for (uint16_t i = 0; i < textLen && xPos < cw; i++) {
        uint8_t c = (uint8_t)cfg.text[i];
        if (c < FONT_FIRST_CHAR || c > 126) c = FONT_FIRST_CHAR;
        uint8_t ci = c - FONT_FIRST_CHAR;

        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++) {
            uint8_t colData = pgm_read_byte(&FONT_5X7[ci][col]);
            for (uint8_t row = 0; row < FONT_CHAR_HEIGHT; row++) {
                if (colData & (1 << row)) {
                    for (uint8_t dy = 0; dy < scale; dy++) {
                        uint16_t y = startY + row * scale + dy;
                        if (y >= ch) break;
                        for (uint8_t dx = 0; dx < scale; dx++) {
                            uint16_t x = xPos + col * scale + dx;
                            if (x < cw)
                                canvas_.setPixel(x, y, cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
                        }
                    }
                }
            }
        }
        xPos += charW + gap;
    }

    transform_.apply(canvas_, fb, cfg);
    fb.swap();
}
