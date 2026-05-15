#pragma once
#include "pattern.h"
#include "../canvas.h"
#include "../transforms/polar_transform.h"

class MatrixPattern : public Pattern {
public:
    MatrixPattern();
    ~MatrixPattern() override { canvas_.release(); }
    void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) override;
    const char* name() const override { return "matrix"; }

private:
    Canvas canvas_;
    PolarTransform transform_;
    uint16_t lastCanvasW_ = 0;
    uint16_t lastCanvasH_ = 0;
    uint16_t lastSlices_ = 0;
    uint16_t lastLeds_ = 0;
    Param storage_[1];

    void ensureCanvas(uint16_t numSlices, uint16_t numLeds);
};
