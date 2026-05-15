#pragma once
#include "pattern.h"
#include "../canvas.h"
#include "../fonts/text_font.h"
#include "../transforms/polar_transform.h"

class TextPattern : public Pattern {
public:
    TextPattern();
    ~TextPattern() override { canvas_.release(); }
    void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) override;
    const char* name() const override { return "text"; }

private:
    Canvas canvas_;
    PolarTransform transform_;
    uint16_t lastCanvasW_ = 0;
    uint16_t lastCanvasH_ = 0;
    uint16_t lastSlices_  = 0;
    uint16_t lastLeds_    = 0;

    char textBuf_[64];
    char cachedText_[64] = {};
    TextFontRun textRun_ = {};
    bool textRunValid_ = false;
    Param storage_[4];

    void ensureCanvas(uint16_t numSlices, uint16_t numLeds);
    void ensureTextRun(const char* text, uint16_t textLen);
};
