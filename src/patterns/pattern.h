#pragma once
#include <cstdint>
#include "../framebuffer.h"
#include "../config.h"

class Pattern {
public:
    virtual ~Pattern() = default;
    virtual void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) = 0;
    virtual const char* name() const = 0;
};
