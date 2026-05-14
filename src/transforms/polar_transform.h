#pragma once
#include "../coord_transform.h"
#include <cstdint>

class PolarTransform : public CoordTransform {
public:
    ~PolarTransform() override { freeLut(); }

    void apply(const Canvas& canvas, Framebuffer& fb, const Config& cfg) override;

    void buildLut(uint16_t numSlices, uint16_t numLeds,
                  uint16_t canvasW, uint16_t canvasH);

private:
    struct LutEntry {
        uint16_t cx;  // canvas x in 8.8 fixed point
        uint16_t cy;  // canvas y in 8.8 fixed point
    };

    LutEntry* lut_       = nullptr;
    uint16_t lutSlices_  = 0;
    uint16_t lutLeds_    = 0;
    uint16_t lutCanvasW_ = 0;
    uint16_t lutCanvasH_ = 0;

    void freeLut();
};
