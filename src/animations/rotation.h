#pragma once
#include "../animation.h"

namespace {
constexpr ParamOption kRotOptions[] = {
    {"Off", 0}, {"Slow", 15}, {"Medium", 45}, {"Fast", 90}
};
}

class RotationAnimation : public Animation {
public:
    RotationAnimation() {
        params_ = storage_;
        paramCount_ = 1;
    }

    const char* name() const override { return "Rotation"; }
    const char* key() const override { return "rot"; }
    bool active() const override { return storage_[0].value != 0; }

    void apply(AnimationState& state, Framebuffer&, uint32_t timeMs) override {
        int32_t degPerSec = storage_[0].value;
        state.sliceOffset += (int16_t)((degPerSec * (int32_t)(timeMs / 10) / 100) % 360);
    }

private:
    Param storage_[1] = {{
        "speed", "Speed", ParamType::Enum,
        0, 0, 0, 90,
        kRotOptions, 4,
        nullptr, 0
    }};
};
