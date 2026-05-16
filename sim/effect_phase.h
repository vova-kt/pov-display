#pragma once

#include <cstdint>
#include "effect.h"

class SimEffectPhase {
public:
    void reset() { sliceOffset_ = 0; }

    void update(const EffectState& state) {
        sliceOffset_ = state.sliceOffset;
    }

    int16_t phaseOffset(int16_t basePhase) const {
        return (int16_t)(basePhase + sliceOffset_);
    }

    int16_t sliceOffset() const { return sliceOffset_; }

private:
    int16_t sliceOffset_ = 0;
};
