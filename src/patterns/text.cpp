#include "text.h"
#include <cstring>

static const ParamOption kTextModeOptions[] = {
    {"Static", 0}, {"Spell", 1}, {"Word", 2}, {"Marquee", 3}
};

// Low-scale odd-length words can put a real glyph over the no-LED center gap.
// Keep this as a named, local layout hack so it is easy to remove when the
// physical pixel count or font strategy changes.
// Q8.8 gives enough intermediate sizes for 20-40 LED canvases without adding
// floating-point work to every text frame on the ESP32-C6.
static constexpr uint16_t kScaleQ8One = 256;
static constexpr uint16_t kCenterDodgeMaxScaleQ8 = 3 * kScaleQ8One;
static constexpr uint8_t kCenterDodgeMinGlyphs = 3;
static constexpr uint8_t kDefaultMarginLeds = 1;
static constexpr uint8_t kMaxMarginLeds = 24;

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
    storage_[3] = {
        "marginLeds", "Margin LEDs", ParamType::Int,
        kDefaultMarginLeds, kDefaultMarginLeds, 0, kMaxMarginLeds,
        nullptr, 0,
        nullptr, 0
    };
    params_ = storage_;
    paramCount_ = 4;
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

static int32_t floorQ8(int32_t v) {
    return v >= 0 ? v / kScaleQ8One : -(((-v) + kScaleQ8One - 1) / kScaleQ8One);
}

static int32_t ceilQ8(int32_t v) {
    return v >= 0 ? (v + kScaleQ8One - 1) / kScaleQ8One : -((-v) / kScaleQ8One);
}

static uint16_t fitScaleQ8(uint16_t pixels, uint16_t sourceUnits) {
    if (sourceUnits == 0) return kScaleQ8One;

    uint32_t scale = (uint32_t)pixels * kScaleQ8One / sourceUnits;
    if (scale > UINT16_MAX) return UINT16_MAX;
    return (uint16_t)scale;
}

static uint16_t clampedInset(uint16_t cw, uint16_t ch, int32_t marginLeds) {
    if (marginLeds <= 0) return 0;

    uint16_t maxX = cw > 1 ? (cw - 1) / 2 : 0;
    uint16_t maxY = ch > 1 ? (ch - 1) / 2 : 0;
    uint16_t maxInset = maxX < maxY ? maxX : maxY;
    return marginLeds > maxInset ? maxInset : (uint16_t)marginLeds;
}

// Sampling each destination pixel at its center keeps fractional glyph scaling
// stable and gap-free while avoiding the code size and SRAM cost of a second
// resampled font buffer.
static void renderGlyph(Canvas& canvas, const TextFontGlyph& glyph,
                        int32_t startXQ8, int32_t startYQ8, uint16_t scaleQ8,
                        uint16_t clipX, uint16_t clipY, uint16_t clipW, uint16_t clipH,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t ch = canvas.height();
    uint16_t cw = canvas.width();
    uint16_t clipRight = clipX + clipW;
    uint16_t clipBottom = clipY + clipH;
    int32_t endXQ8 = startXQ8 + (int32_t)glyph.width * scaleQ8;
    int32_t endYQ8 = startYQ8 + (int32_t)TEXT_FONT_METRICS.height * scaleQ8;

    int32_t minX = floorQ8(startXQ8);
    int32_t maxX = ceilQ8(endXQ8);
    int32_t minY = floorQ8(startYQ8);
    int32_t maxY = ceilQ8(endYQ8);

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX > (int32_t)cw) maxX = cw;
    if (maxY > (int32_t)ch) maxY = ch;
    if (minX < (int32_t)clipX) minX = clipX;
    if (minY < (int32_t)clipY) minY = clipY;
    if (maxX > (int32_t)clipRight) maxX = clipRight;
    if (maxY > (int32_t)clipBottom) maxY = clipBottom;

    for (int32_t y = minY; y < maxY; y++) {
        int32_t relYQ8 = (y * kScaleQ8One + kScaleQ8One / 2) - startYQ8;
        if (relYQ8 < 0) continue;
        uint8_t row = (uint8_t)((uint32_t)relYQ8 / scaleQ8);
        if (row >= TEXT_FONT_METRICS.height) continue;

        for (int32_t x = minX; x < maxX; x++) {
            int32_t relXQ8 = (x * kScaleQ8One + kScaleQ8One / 2) - startXQ8;
            if (relXQ8 < 0) continue;
            uint8_t col = (uint8_t)((uint32_t)relXQ8 / scaleQ8);
            if (col >= glyph.width) continue;

            if (glyph.cols[col] & (1 << row))
                canvas.setPixel((uint16_t)x, (uint16_t)y, r, g, b, brightness);
        }
    }
}

static void renderLine(Canvas& canvas, const TextFontRunView& run,
                       int32_t startXQ8, int32_t startYQ8, uint16_t scaleQ8,
                       uint16_t clipX, uint16_t clipY, uint16_t clipW, uint16_t clipH,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint16_t clipRight = clipX + clipW;
    int32_t xPosQ8 = startXQ8;

    for (uint8_t i = 0; i < run.count; i++) {
        if (xPosQ8 >= (int32_t)clipRight * kScaleQ8One) break;

        const TextFontGlyph& glyph = run.glyphs[i].glyph;
        int32_t glyphWQ8 = (int32_t)glyph.width * scaleQ8;
        if (xPosQ8 + glyphWQ8 >= (int32_t)clipX * kScaleQ8One)
            renderGlyph(canvas, glyph, xPosQ8, startYQ8, scaleQ8,
                        clipX, clipY, clipW, clipH, r, g, b, brightness);
        xPosQ8 += glyphWQ8 + (int32_t)TEXT_FONT_METRICS.gap * scaleQ8;
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

static bool canCenterDodge(const TextFontRunView& run) {
    if (run.count < kCenterDodgeMinGlyphs) return false;
    if ((run.count & 1) == 0) return false;

    const TextFontRunGlyph& centerGlyph = run.glyphs[run.count / 2];
    return !centerGlyph.isSpace;
}

static uint32_t centerDodgeNudgeQ8(const TextFontRunView& run, uint16_t scaleQ8) {
    const TextFontRunGlyph& centerGlyph = run.glyphs[run.count / 2];
    return (uint32_t)(centerGlyph.glyph.width + TEXT_FONT_METRICS.gap) * scaleQ8 / 2;
}

// Reserve dodge slack during fitting so odd short words move away from the hub
// without later clipping at the outer edge.
static uint16_t horizontalFitScaleQ8(uint16_t cw, const TextFontRunView& run) {
    uint16_t scaleQ8 = fitScaleQ8(cw, run.width);

    if (scaleQ8 <= kCenterDodgeMaxScaleQ8 && canCenterDodge(run)) {
        const TextFontRunGlyph& centerGlyph = run.glyphs[run.count / 2];
        uint16_t widthWithDodgeRoom = run.width + centerGlyph.glyph.width + TEXT_FONT_METRICS.gap;
        scaleQ8 = fitScaleQ8(cw, widthWithDodgeRoom);
    }

    return scaleQ8;
}

static int32_t centerDodgeOffsetQ8(uint16_t cw, uint32_t totalWQ8, int32_t startXQ8,
                                   const TextFontRunView& run, uint16_t scaleQ8) {
    if (scaleQ8 > kCenterDodgeMaxScaleQ8) return 0;
    if (!canCenterDodge(run)) return 0;

    uint32_t nudgeQ8 = centerDodgeNudgeQ8(run, scaleQ8);
    if (nudgeQ8 == 0) return 0;

    int32_t cwQ8 = (int32_t)cw * kScaleQ8One;
    int32_t rightEdgeQ8 = startXQ8 > 0 ? startXQ8 + (int32_t)totalWQ8 : (int32_t)totalWQ8;
    uint32_t rightRoomQ8 = rightEdgeQ8 < cwQ8 ? (uint32_t)(cwQ8 - rightEdgeQ8) : 0;
    if (rightRoomQ8 >= nudgeQ8) return (int32_t)nudgeQ8;

    uint32_t leftRoomQ8 = startXQ8 > 0 ? (uint32_t)startXQ8 : 0;
    if (leftRoomQ8 >= nudgeQ8) return -(int32_t)nudgeQ8;

    return 0;
}

static void computeScale(uint16_t cw, uint16_t ch, const TextFontRunView& run,
                         TextFontRunView lines[2], uint8_t& numLines,
                         uint16_t& scaleQ8) {
    lines[0] = run;
    lines[1] = {nullptr, 0, 0};
    numLines = 1;

    uint16_t scaleVQ8 = fitScaleQ8(ch, TEXT_FONT_METRICS.height);
    uint16_t scaleHQ8 = horizontalFitScaleQ8(cw, run);
    scaleQ8 = scaleVQ8 < scaleHQ8 ? scaleVQ8 : scaleHQ8;

    // Keep short text on one line whenever 1x still fits; wrapping earlier
    // makes the POV image read as two unrelated fragments.
    if (scaleQ8 < kScaleQ8One) {
        uint16_t bestScaleHQ8 = 0;
        int bestBreak = -1;

        for (uint8_t i = 1; i < run.count; i++) {
            if (!run.glyphs[i].isSpace) continue;
            uint8_t l1 = i;
            uint8_t l2 = run.count - i - 1;
            if (l2 == 0) continue;

            TextFontRunView line1 = textFontRunView(run.glyphs, l1);
            TextFontRunView line2 = textFontRunView(run.glyphs + i + 1, l2);
            uint16_t h1Q8 = horizontalFitScaleQ8(cw, line1);
            uint16_t h2Q8 = horizontalFitScaleQ8(cw, line2);
            uint16_t candidateQ8 = h1Q8 < h2Q8 ? h1Q8 : h2Q8;

            if (candidateQ8 > bestScaleHQ8) {
                bestScaleHQ8 = candidateQ8;
                bestBreak = i;
            }
        }

        if (bestBreak >= 0) {
            uint8_t l1 = (uint8_t)bestBreak;
            uint8_t l2 = run.count - (uint8_t)bestBreak - 1;
            lines[0] = textFontRunView(run.glyphs, l1);
            lines[1] = textFontRunView(run.glyphs + bestBreak + 1, l2);
            numLines = 2;

            scaleVQ8 = fitScaleQ8(ch, 2 * TEXT_FONT_METRICS.height + TEXT_FONT_METRICS.gap);
            scaleQ8 = scaleVQ8 < bestScaleHQ8 ? scaleVQ8 : bestScaleHQ8;
        }
    }

    if (scaleQ8 < kScaleQ8One) scaleQ8 = kScaleQ8One;
}

static void renderCentered(Canvas& canvas, const TextFontRunView lines[2],
                           uint16_t originX, uint16_t originY,
                           uint16_t layoutW, uint16_t layoutH,
                           uint8_t numLines, uint16_t scaleQ8,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint32_t lineHQ8   = (uint32_t)TEXT_FONT_METRICS.height * scaleQ8;
    uint32_t lineGapQ8 = numLines > 1 ? scaleQ8 : 0;
    uint32_t totalHQ8  = numLines * lineHQ8 + (numLines - 1) * lineGapQ8;
    uint32_t chQ8      = (uint32_t)layoutH * kScaleQ8One;
    int32_t startYQ8   = totalHQ8 < chQ8 ? (int32_t)((chQ8 - totalHQ8) / 2) : 0;

    for (uint8_t line = 0; line < numLines; line++) {
        uint32_t totalWQ8 = (uint32_t)lines[line].width * scaleQ8;
        uint32_t cwQ8 = (uint32_t)layoutW * kScaleQ8One;
        int32_t startXQ8 = totalWQ8 < cwQ8 ? (int32_t)((cwQ8 - totalWQ8) / 2) : 0;
        startXQ8 += centerDodgeOffsetQ8(layoutW, totalWQ8, startXQ8, lines[line], scaleQ8);
        startXQ8 += (int32_t)originX * kScaleQ8One;
        int32_t yQ8 = (int32_t)originY * kScaleQ8One + startYQ8 +
                      (int32_t)((uint32_t)line * (lineHQ8 + lineGapQ8));

        renderLine(canvas, lines[line], startXQ8, yQ8, scaleQ8,
                   originX, originY, layoutW, layoutH,
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
    int32_t marginLeds = storage_[3].value;

    ensureCanvas(fb.numSlices(), fb.numLeds());
    canvas_.clear();

    uint16_t cw = canvas_.width();
    uint16_t ch = canvas_.height();
    uint16_t inset = clampedInset(cw, ch, marginLeds);
    uint16_t layoutW = cw - 2 * inset;
    uint16_t layoutH = ch - 2 * inset;

    if (mode == 3) {
        // Marquee stays integer-scaled so scrolling advances on whole source
        // pixels; fractional steps make the motion shimmer more than they help.
        uint8_t scale = layoutH / TEXT_FONT_METRICS.height;
        if (scale < 1) scale = 1;
        uint16_t scaleQ8 = (uint16_t)scale * kScaleQ8One;
        uint16_t textW = (uint16_t)(((uint32_t)textRun_.width * scaleQ8 + kScaleQ8One - 1) / kScaleQ8One);
        uint16_t scrollRange = textW + layoutW;
        uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
        uint16_t pixPerStep = scale;
        uint32_t stepMs = delayMs / pixPerStep;
        if (stepMs < 1) stepMs = 1;
        int16_t offset = (int16_t)((timeMs / stepMs) % scrollRange);
        int16_t startX = (int16_t)(inset + layoutW) - offset;
        uint16_t startY = layoutH > (TEXT_FONT_METRICS.height * scale)
                          ? inset + (layoutH - TEXT_FONT_METRICS.height * scale) / 2 : inset;

        renderLine(canvas_, textFontRunView(textRun_),
                   (int32_t)startX * kScaleQ8One, (int32_t)startY * kScaleQ8One, scaleQ8,
                   inset, inset, layoutW, layoutH,
                   cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    } else {
        TextFontRunView fullRun = textFontRunView(textRun_);
        TextFontRunView visible = fullRun;

        if (mode == 1) {
            // Spell mode isolates each glyph so tiny displays can give every
            // character the full centered fit instead of shrinking the word.
            uint16_t delayMs = delayMsIn > 0 ? (uint16_t)delayMsIn : 500;
            uint16_t cycle = textRun_.count + 2;
            uint16_t step = (uint16_t)((timeMs / delayMs) % cycle);
            if (step < textRun_.count)
                visible = textFontRunView(textRun_.glyphs + step, 1);
            else
                visible = {nullptr, 0, 0};
        } else if (mode == 2) {
            // Word mode centers each word independently; accumulating prefixes
            // made early words large and later words abruptly tiny.
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
        uint16_t scaleQ8;
        computeScale(layoutW, layoutH, visible, lines, numLines, scaleQ8);
        renderCentered(canvas_, lines, inset, inset, layoutW, layoutH, numLines, scaleQ8,
                       cfg.colorR, cfg.colorG, cfg.colorB, cfg.brightness);
    }

    transform_.apply(canvas_, fb, cfg);
}
