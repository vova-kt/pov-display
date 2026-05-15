#pragma once
#include <cstdint>
#include <cstring>
#include "param.h"

class Framebuffer;

struct AnimationState {
    int16_t sliceOffset = 0;
};

constexpr uint8_t G_NUM_ANIMATION_SLOTS = 2;

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

int8_t animationIndexForKey(const char* key);
const char* animationSlotKey(uint8_t slot);
bool setAnimationSlot(uint8_t slot, const char* key);
void resetAnimationStackDefaults();

void loadAnimationsFromNvs();
void saveAnimationsToNvs();

extern Animation* const g_animations[];
extern const uint8_t G_NUM_ANIMATIONS;
extern int8_t g_animationStack[G_NUM_ANIMATION_SLOTS];
