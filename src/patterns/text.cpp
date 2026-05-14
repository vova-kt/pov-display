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

static uint16_t lineBaseCols(uint16_t len) {
    return len * (FONT_CHAR_WIDTH + 1) - 1;
}

static void renderLine(Canvas& canvas, const char* text, uint16_t len,
                       uint16_t startX, uint16_t startY, uint8_t scale,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t cw = canvas.width();
    uint16_t ch = canvas.height();
    uint16_t charW = FONT_CHAR_WIDTH * scale;
    uint16_t gap   = scale;
    uint16_t xPos  = startX;

    for (uint16_t i = 0; i < len && xPos < cw; i++) {
        uint8_t c = (uint8_t)text[i];
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
                                canvas.setPixel(x, y, r, g, b, brightness);
                        }
                    }
                }
            }
        }
        xPos += charW + gap;
    }
}

void TextPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t) {
    fb.clearBack();

    uint16_t textLen = strlen(cfg.text);
    if (textLen == 0) { fb.swap(); return; }

    ensureCanvas(fb.numSlices(), fb.numLeds());
    canvas_.clear();

    uint16_t cw = canvas_.width();
    uint16_t ch = canvas_.height();

    const char* lines[2] = { cfg.text, nullptr };
    uint16_t    lens[2]  = { textLen, 0 };
    uint8_t numLines = 1;

    uint16_t baseCols = lineBaseCols(textLen);
    uint8_t scaleV = ch / FONT_CHAR_HEIGHT;
    uint8_t scaleH = baseCols <= cw ? cw / baseCols : 0;
    uint8_t scale  = scaleV < scaleH ? scaleV : scaleH;

    if (scale < 1) {
        // Text doesn't fit on one line — find best space to break at (max 2 lines)
        uint16_t bestMaxBC = UINT16_MAX;
        int bestBreak = -1;

        for (uint16_t i = 1; i < textLen; i++) {
            if (cfg.text[i] != ' ') continue;
            uint16_t l1 = i;
            uint16_t l2 = textLen - i - 1;
            if (l2 == 0) continue;

            uint16_t bc1 = lineBaseCols(l1);
            uint16_t bc2 = lineBaseCols(l2);
            uint16_t maxBC = bc1 > bc2 ? bc1 : bc2;

            if (maxBC < bestMaxBC) {
                bestMaxBC = maxBC;
                bestBreak = i;
            }
        }

        if (bestBreak >= 0) {
            lens[0]  = bestBreak;
            lines[1] = cfg.text + bestBreak + 1;
            lens[1]  = textLen - bestBreak - 1;
            numLines = 2;

            scaleV = ch / (2 * FONT_CHAR_HEIGHT + 1);
            scaleH = bestMaxBC <= cw ? cw / bestMaxBC : 0;
            scale  = scaleV < scaleH ? scaleV : scaleH;
        }
    }

    if (scale < 1) scale = 1;

    uint16_t charW    = FONT_CHAR_WIDTH * scale;
    uint16_t gap      = scale;
    uint16_t lineH    = FONT_CHAR_HEIGHT * scale;
    uint16_t lineGap  = numLines > 1 ? scale : 0;
    uint16_t totalH   = numLines * lineH + (numLines - 1) * lineGap;

    uint16_t startY = totalH < ch ? (ch - totalH) / 2 : 0;

    for (uint8_t line = 0; line < numLines; line++) {
        uint16_t totalW = lens[line] * (charW + gap) - gap;
        uint16_t startX = totalW < cw ? (cw - totalW) / 2 : 0;
        uint16_t y = startY + line * (lineH + lineGap);

        renderLine(canvas_, lines[line], lens[line], startX, y, scale,
                   cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    }

    transform_.apply(canvas_, fb, cfg);
    fb.swap();
}
