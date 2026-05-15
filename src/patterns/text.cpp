#include "text.h"
#include "../fonts/font5x7.h"
#include <cstring>
#include <pgmspace.h>

static const ParamOption kTextModeOptions[] = {
    {"Static", 0}, {"Spell", 1}, {"Word", 2}, {"Marquee", 3}
};

TextPattern::TextPattern() {
    strcpy(textBuf_, "FUSION");
    storage_[0] = {
        "text", "Text", ParamType::Text,
        0, 0, 0, 0,
        nullptr, 0,
        textBuf_, sizeof(textBuf_)
    };
    storage_[1] = {
        "mode", "Mode", ParamType::Enum,
        0, 0, 0, 3,
        kTextModeOptions, 4,
        nullptr, 0
    };
    storage_[2] = {
        "delayMs", "Delay ms", ParamType::Int,
        500, 500, 50, 2000,
        nullptr, 0,
        nullptr, 0
    };
    params_ = storage_;
    paramCount_ = 3;
}

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
                       int16_t startX, uint16_t startY, uint8_t scale,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t cw = canvas.width();
    uint16_t ch = canvas.height();
    uint16_t charW = FONT_CHAR_WIDTH * scale;
    uint16_t gap   = scale;
    int16_t xPos   = startX;

    for (uint16_t i = 0; i < len; i++) {
        if (xPos >= (int16_t)cw) break;
        if (xPos + (int16_t)charW < 0) { xPos += charW + gap; continue; }

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
                            int16_t x = xPos + col * scale + dx;
                            if (x >= 0 && x < (int16_t)cw)
                                canvas.setPixel((uint16_t)x, y, r, g, b, brightness);
                        }
                    }
                }
            }
        }
        xPos += charW + gap;
    }
}

static uint16_t countWords(const char* text, uint16_t len) {
    if (len == 0) return 0;
    uint16_t words = 1;
    for (uint16_t i = 0; i < len; i++)
        if (text[i] == ' ' && i + 1 < len) words++;
    return words;
}

static uint16_t charsForWords(const char* text, uint16_t len, uint16_t wordCount) {
    if (wordCount == 0) return 0;
    uint16_t w = 0;
    for (uint16_t i = 0; i < len; i++) {
        if (i == 0 || text[i - 1] == ' ') w++;
        if (w > wordCount) return i - 1;
    }
    return len;
}

static void computeScale(uint16_t cw, uint16_t ch, const char* text, uint16_t textLen,
                          const char* lines[2], uint16_t lens[2], uint8_t& numLines,
                          uint8_t& scale) {
    lines[0] = text;
    lines[1] = nullptr;
    lens[0]  = textLen;
    lens[1]  = 0;
    numLines = 1;

    uint16_t baseCols = lineBaseCols(textLen);
    uint8_t scaleV = ch / FONT_CHAR_HEIGHT;
    uint8_t scaleH = baseCols <= cw ? cw / baseCols : 0;
    scale = scaleV < scaleH ? scaleV : scaleH;

    if (scale < 1) {
        uint16_t bestMaxBC = UINT16_MAX;
        int bestBreak = -1;

        for (uint16_t i = 1; i < textLen; i++) {
            if (text[i] != ' ') continue;
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
            lines[1] = text + bestBreak + 1;
            lens[1]  = textLen - bestBreak - 1;
            numLines = 2;

            scaleV = ch / (2 * FONT_CHAR_HEIGHT + 1);
            scaleH = bestMaxBC <= cw ? cw / bestMaxBC : 0;
            scale  = scaleV < scaleH ? scaleV : scaleH;
        }
    }

    if (scale < 1) scale = 1;
}

static void renderCentered(Canvas& canvas, const char* lines[2], uint16_t lens[2],
                            uint8_t numLines, uint8_t scale,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t cw = canvas.width();
    uint16_t ch = canvas.height();
    uint16_t charW   = FONT_CHAR_WIDTH * scale;
    uint16_t gap     = scale;
    uint16_t lineH   = FONT_CHAR_HEIGHT * scale;
    uint16_t lineGap = numLines > 1 ? scale : 0;
    uint16_t totalH  = numLines * lineH + (numLines - 1) * lineGap;
    uint16_t startY  = totalH < ch ? (ch - totalH) / 2 : 0;

    for (uint8_t line = 0; line < numLines; line++) {
        uint16_t totalW = lens[line] * (charW + gap) - gap;
        int16_t startX = totalW < cw ? (int16_t)((cw - totalW) / 2) : 0;
        uint16_t y = startY + line * (lineH + lineGap);

        renderLine(canvas, lines[line], lens[line], startX, y, scale,
                   r, g, b, brightness);
    }
}

void TextPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) {
    fb.clearBack();

    const char* text = textBuf_;
    uint16_t textLen = strlen(text);
    if (textLen == 0) return;

    int32_t mode      = storage_[1].value;
    int32_t delayMsIn = storage_[2].value;

    ensureCanvas(fb.numSlices(), fb.numLeds());
    canvas_.clear();

    uint16_t cw = canvas_.width();
    uint16_t ch = canvas_.height();

    if (mode == 3) {
        // Marquee: single-line scroll, scale to fit vertically
        uint8_t scale = ch / FONT_CHAR_HEIGHT;
        if (scale < 1) scale = 1;
        uint16_t charW = FONT_CHAR_WIDTH * scale;
        uint16_t gap   = scale;
        uint16_t textW = textLen * (charW + gap);
        uint16_t scrollRange = textW + cw;
        uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
        uint16_t pixPerStep = scale;
        uint32_t stepMs = delayMs / pixPerStep;
        if (stepMs < 1) stepMs = 1;
        int16_t offset = (int16_t)((timeMs / stepMs) % scrollRange);
        int16_t startX = (int16_t)cw - offset;
        uint16_t startY = ch > (FONT_CHAR_HEIGHT * scale)
                          ? (ch - FONT_CHAR_HEIGHT * scale) / 2 : 0;

        renderLine(canvas_, text, textLen, startX, startY, scale,
                   cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    } else {
        uint16_t visLen = textLen;

        uint16_t visOffset = 0;

        if (mode == 1) {
            // Spell: one character at a time
            uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
            uint16_t cycle = textLen + 2;
            uint16_t step = (uint16_t)((timeMs / delayMs) % cycle);
            if (step < textLen) {
                visOffset = step;
                visLen = 1;
            } else {
                visLen = 0;
            }
        } else if (mode == 2) {
            // Word by word
            uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
            uint16_t totalWords = countWords(text, textLen);
            uint16_t cycle = totalWords + 2;
            uint16_t step = (uint16_t)((timeMs / delayMs) % cycle);
            uint16_t visWords = step < totalWords ? step + 1 : totalWords;
            visLen = charsForWords(text, textLen, visWords);
        }

        if (visLen == 0) {
            transform_.apply(canvas_, fb, cfg);
            return;
        }

        // Static buffer for visible substring
        char visBuf[64];
        const char* visText;
        if (visLen < textLen || visOffset > 0) {
            memcpy(visBuf, text + visOffset, visLen);
            visBuf[visLen] = '\0';
            visText = visBuf;
        } else {
            visText = text;
        }

        const char* lines[2];
        uint16_t lens[2];
        uint8_t numLines;
        uint8_t scale;
        computeScale(cw, ch, visText, visLen, lines, lens, numLines, scale);
        renderCentered(canvas_, lines, lens, numLines, scale,
                       cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    }

    transform_.apply(canvas_, fb, cfg);
}
