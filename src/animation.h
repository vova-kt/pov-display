#pragma once
#include <cstdint>
#include <cstring>
#include "param.h"

class Framebuffer;

struct AnimationState {
    int16_t sliceOffset = 0;
};

class Animation {
public:
    virtual ~Animation() = default;
    virtual const char* name() const = 0;
    virtual const char* key() const = 0;
    virtual bool active() const = 0;
    virtual void apply(AnimationState& state, Framebuffer& fb, uint32_t timeMs) = 0;

    uint8_t paramCount() const { return paramCount_; }
    Param& param(uint8_t i) { return params_[i]; }
    const Param& param(uint8_t i) const { return params_[i]; }

    Param* findParam(const char* paramKey) {
        for (uint8_t i = 0; i < paramCount_; i++)
            if (strcmp(params_[i].key, paramKey) == 0) return &params_[i];
        return nullptr;
    }

    void resetDefaults() {
        for (uint8_t i = 0; i < paramCount_; i++)
            params_[i].value = params_[i].defaultVal;
    }

protected:
    Param*  params_ = nullptr;
    uint8_t paramCount_ = 0;
};

void applyAnimations(AnimationState& state, Framebuffer& fb, uint32_t timeMs);

void loadAnimationsFromNvs();
void saveAnimationsToNvs();

extern Animation* const g_animations[];
extern const uint8_t G_NUM_ANIMATIONS;
