#include "animation.h"
#include "animations/rotation.h"

static RotationAnimation s_rotation;

Animation* const g_animations[] = { &s_rotation };
const uint8_t G_NUM_ANIMATIONS = sizeof(g_animations) / sizeof(g_animations[0]);

void applyAnimations(AnimationState& state, Framebuffer& fb, uint32_t timeMs) {
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++) {
        if (g_animations[i]->active())
            g_animations[i]->apply(state, fb, timeMs);
    }
}
