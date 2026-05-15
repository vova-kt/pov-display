#pragma once
#include <cstdint>
#include <cstring>

class Framebuffer;

struct AnimPreset {
    const char* label;
    int16_t value;
};

struct AnimParam {
    const char* key;
    const char* label;
    int16_t value;
    int16_t defaultVal;
    int16_t min;
    int16_t max;
    const AnimPreset* presets;
    uint8_t presetCount;
};

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
    AnimParam& param(uint8_t i) { return params_[i]; }
    const AnimParam& param(uint8_t i) const { return params_[i]; }

    AnimParam* findParam(const char* paramKey) {
        for (uint8_t i = 0; i < paramCount_; i++)
            if (strcmp(params_[i].key, paramKey) == 0) return &params_[i];
        return nullptr;
    }

    void resetDefaults() {
        for (uint8_t i = 0; i < paramCount_; i++)
            params_[i].value = params_[i].defaultVal;
    }

protected:
    AnimParam* params_ = nullptr;
    uint8_t paramCount_ = 0;
};

void applyAnimations(AnimationState& state, Framebuffer& fb, uint32_t timeMs);

void loadAnimationsFromNvs();
void saveAnimationsToNvs();

extern Animation* const g_animations[];
extern const uint8_t G_NUM_ANIMATIONS;
