#pragma once
#include "pattern.h"

class SolidPattern : public Pattern {
public:
    void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) override;
    const char* name() const override { return "solid"; }
};
