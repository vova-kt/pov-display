#include "text.h"
#include <cstring>

static const ParamOption kTextModeOptions[] = {
    {"Static", 0}, {"Spell", 1}, {"Word", 2}, {"Marquee", 3}
};

// Low-scale odd-length words can put a real glyph over the no-LED center gap.
// Keep this as a named, local layout hack so it is easy to remove when the
// physical pixel count or font strategy changes.
static constexpr uint8_t kCenterDodgeMaxScale = 2;
static constexpr uint8_t kCenterDodgeMinGlyphs = 3;

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

void TextPattern::ensureTextRun(const char* text, uint16_t textLen) {
    if (textRunValid_ && strcmp(cachedText_, text) == 0) return;

    textFontDecodeRun(text, textLen, textRun_);
    strncpy(cachedText_, text, sizeof(cachedText_) - 1);
    cachedText_[sizeof(cachedText_) - 1] = '\0';
    textRunValid_ = true;
}

static void renderGlyph(Canvas& canvas, const TextFontGlyph& glyph,
                        int16_t startX, uint16_t startY, uint8_t scale,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t ch = canvas.height();
    uint16_t cw = canvas.width();

    for (uint8_t col = 0; col < glyph.width; col++) {
        uint8_t colData = glyph.cols[col];
        for (uint8_t row = 0; row < TEXT_FONT_METRICS.height; row++) {
            if (!(colData & (1 << row))) continue;

            for (uint8_t dy = 0; dy < scale; dy++) {
                uint16_t y = startY + row * scale + dy;
                if (y >= ch) break;

                for (uint8_t dx = 0; dx < scale; dx++) {
                    int16_t x = startX + col * scale + dx;
                    if (x >= 0 && x < (int16_t)cw)
                        canvas.setPixel((uint16_t)x, y, r, g, b, brightness);
                }
            }
        }
    }
}

static void renderLine(Canvas& canvas, const TextFontRunView& run,
                       int16_t startX, uint16_t startY, uint8_t scale,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t cw = canvas.width();
    int16_t xPos = startX;

    for (uint8_t i = 0; i < run.count; i++) {
        if (xPos >= (int16_t)cw) break;

        const TextFontGlyph& glyph = run.glyphs[i].glyph;
        uint16_t glyphW = glyph.width * scale;
        if (xPos + (int16_t)glyphW >= 0)
            renderGlyph(canvas, glyph, xPos, startY, scale, r, g, b, brightness);
        xPos += glyphW + TEXT_FONT_METRICS.gap * scale;
    }
}

static uint8_t countWords(const TextFontRunView& run) {
    uint8_t words = 0;
    bool inWord = false;
    for (uint8_t i = 0; i < run.count; i++) {
        if (run.glyphs[i].isSpace) {
            inWord = false;
        } else if (!inWord) {
            words++;
            inWord = true;
        }
    }
    return words;
}

static TextFontRunView wordRunAt(const TextFontRunView& run, uint8_t wordIndex) {
    uint8_t currentWord = 0;
    uint8_t i = 0;

    while (i < run.count) {
        while (i < run.count && run.glyphs[i].isSpace) i++;
        if (i >= run.count) break;

        uint8_t start = i;
        while (i < run.count && !run.glyphs[i].isSpace) i++;

        if (currentWord == wordIndex)
            return textFontRunView(run.glyphs + start, i - start);
        currentWord++;
    }

    return {nullptr, 0, 0};
}

static int16_t centerDodgeOffset(uint16_t cw, uint16_t totalW, int16_t startX,
                                 const TextFontRunView& run, uint8_t scale) {
    if (scale > kCenterDodgeMaxScale) return 0;
    if (run.count < kCenterDodgeMinGlyphs) return 0;
    if ((run.count & 1) == 0) return 0;

    const TextFontRunGlyph& centerGlyph = run.glyphs[run.count / 2];
    if (centerGlyph.isSpace) return 0;

    uint16_t nudge = ((uint16_t)centerGlyph.glyph.width + TEXT_FONT_METRICS.gap) * scale / 2;
    if (nudge == 0) return 0;

    uint16_t rightEdge = startX > 0 ? (uint16_t)startX + totalW : totalW;
    uint16_t rightRoom = rightEdge < cw ? cw - rightEdge : 0;
    if (rightRoom >= nudge) return (int16_t)nudge;

    uint16_t leftRoom = startX > 0 ? (uint16_t)startX : 0;
    if (leftRoom >= nudge) return -(int16_t)nudge;

    return 0;
}

static void computeScale(uint16_t cw, uint16_t ch, const TextFontRunView& run,
                         TextFontRunView lines[2], uint8_t& numLines,
                         uint8_t& scale) {
    lines[0] = run;
    lines[1] = {nullptr, 0, 0};
    numLines = 1;

    uint16_t baseCols = run.width;
    uint8_t scaleV = ch / TEXT_FONT_METRICS.height;
    uint8_t scaleH = baseCols <= cw ? cw / baseCols : 0;
    scale = scaleV < scaleH ? scaleV : scaleH;

    if (scale < 1) {
        uint16_t bestMaxBC = UINT16_MAX;
        int bestBreak = -1;

        for (uint8_t i = 1; i < run.count; i++) {
            if (!run.glyphs[i].isSpace) continue;
            uint8_t l1 = i;
            uint8_t l2 = run.count - i - 1;
            if (l2 == 0) continue;

            uint16_t bc1 = textFontMeasureGlyphs(run.glyphs, l1);
            uint16_t bc2 = textFontMeasureGlyphs(run.glyphs + i + 1, l2);
            uint16_t maxBC = bc1 > bc2 ? bc1 : bc2;

            if (maxBC < bestMaxBC) {
                bestMaxBC = maxBC;
                bestBreak = i;
            }
        }

        if (bestBreak >= 0) {
            uint8_t l1 = (uint8_t)bestBreak;
            uint8_t l2 = run.count - (uint8_t)bestBreak - 1;
            lines[0] = textFontRunView(run.glyphs, l1);
            lines[1] = textFontRunView(run.glyphs + bestBreak + 1, l2);
            numLines = 2;

            scaleV = ch / (2 * TEXT_FONT_METRICS.height + TEXT_FONT_METRICS.gap);
            scaleH = bestMaxBC <= cw ? cw / bestMaxBC : 0;
            scale  = scaleV < scaleH ? scaleV : scaleH;
        }
    }

    if (scale < 1) scale = 1;
}

static void renderCentered(Canvas& canvas, const TextFontRunView lines[2],
                           uint8_t numLines, uint8_t scale,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t cw = canvas.width();
    uint16_t ch = canvas.height();
    uint16_t lineH   = TEXT_FONT_METRICS.height * scale;
    uint16_t lineGap = numLines > 1 ? scale : 0;
    uint16_t totalH  = numLines * lineH + (numLines - 1) * lineGap;
    uint16_t startY  = totalH < ch ? (ch - totalH) / 2 : 0;

    for (uint8_t line = 0; line < numLines; line++) {
        uint16_t totalW = lines[line].width * scale;
        int16_t startX = totalW < cw ? (int16_t)((cw - totalW) / 2) : 0;
        startX += centerDodgeOffset(cw, totalW, startX, lines[line], scale);
        uint16_t y = startY + line * (lineH + lineGap);

        renderLine(canvas, lines[line], startX, y, scale,
                   r, g, b, brightness);
    }
}

void TextPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) {
    fb.clearBack();

    const char* text = textBuf_;
    uint16_t textLen = strlen(text);
    if (textLen == 0) return;

    ensureTextRun(text, textLen);
    if (textRun_.count == 0) return;

    int32_t mode      = storage_[1].value;
    int32_t delayMsIn = storage_[2].value;

    ensureCanvas(fb.numSlices(), fb.numLeds());
    canvas_.clear();

    uint16_t cw = canvas_.width();
    uint16_t ch = canvas_.height();

    if (mode == 3) {
        // Marquee: single-line scroll, scale to fit vertically
        uint8_t scale = ch / TEXT_FONT_METRICS.height;
        if (scale < 1) scale = 1;
        uint16_t textW = textRun_.width * scale;
        uint16_t scrollRange = textW + cw;
        uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
        uint16_t pixPerStep = scale;
        uint32_t stepMs = delayMs / pixPerStep;
        if (stepMs < 1) stepMs = 1;
        int16_t offset = (int16_t)((timeMs / stepMs) % scrollRange);
        int16_t startX = (int16_t)cw - offset;
        uint16_t startY = ch > (TEXT_FONT_METRICS.height * scale)
                          ? (ch - TEXT_FONT_METRICS.height * scale) / 2 : 0;

        renderLine(canvas_, textFontRunView(textRun_), startX, startY, scale,
                   cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    } else {
        TextFontRunView fullRun = textFontRunView(textRun_);
        TextFontRunView visible = fullRun;

        if (mode == 1) {
            // Spell: one character at a time
            uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
            uint16_t cycle = textRun_.count + 2;
            uint16_t step = (uint16_t)((timeMs / delayMs) % cycle);
            if (step < textRun_.count)
                visible = textFontRunView(textRun_.glyphs + step, 1);
            else
                visible = {nullptr, 0, 0};
        } else if (mode == 2) {
            // Word: one non-space run at a time, with a short blank pause.
            uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
            uint8_t totalWords = countWords(fullRun);
            uint16_t cycle = totalWords + 2;
            uint16_t step = (uint16_t)((timeMs / delayMs) % cycle);
            visible = step < totalWords ? wordRunAt(fullRun, (uint8_t)step)
                                        : TextFontRunView{nullptr, 0, 0};
        }

        if (visible.count == 0) {
            transform_.apply(canvas_, fb, cfg);
            return;
        }

        TextFontRunView lines[2];
        uint8_t numLines;
        uint8_t scale;
        computeScale(cw, ch, visible, lines, numLines, scale);
        renderCentered(canvas_, lines, numLines, scale,
                       cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    }

    transform_.apply(canvas_, fb, cfg);
}
