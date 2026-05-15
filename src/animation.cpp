#include "animation.h"
#include "animations/rotation.h"
#include "animations/scale.h"

static RotationAnimation s_rotation;
static ScaleAnimation s_scale;

Animation* const g_animations[] = { &s_rotation, &s_scale };
const uint8_t G_NUM_ANIMATIONS = sizeof(g_animations) / sizeof(g_animations[0]);
int8_t g_animationStack[G_NUM_ANIMATION_SLOTS] = {0, -1};

int8_t animationIndexForKey(const char* key) {
    if (!key || key[0] == '\0') return -1;
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++) {
        if (strcmp(g_animations[i]->key(), key) == 0) return (int8_t)i;
    }
    return -1;
}

const char* animationSlotKey(uint8_t slot) {
    if (slot >= G_NUM_ANIMATION_SLOTS) return "";
    int8_t idx = g_animationStack[slot];
    if (idx < 0 || idx >= (int8_t)G_NUM_ANIMATIONS) return "";
    return g_animations[idx]->key();
}

bool setAnimationSlot(uint8_t slot, const char* key) {
    if (slot >= G_NUM_ANIMATION_SLOTS) return false;
    if (!key || key[0] == '\0' || strcmp(key, "none") == 0) {
        g_animationStack[slot] = -1;
        return true;
    }

    int8_t idx = animationIndexForKey(key);
    if (idx < 0) return false;

    g_animationStack[slot] = idx;
    return true;
}

void resetAnimationStackDefaults() {
    g_animationStack[0] = animationIndexForKey("rot");
    for (uint8_t i = 1; i < G_NUM_ANIMATION_SLOTS; i++)
        g_animationStack[i] = -1;
}

void applyAnimations(AnimationState& state, Framebuffer& fb, uint32_t timeMs) {
    for (uint8_t slot = 0; slot < G_NUM_ANIMATION_SLOTS; slot++) {
        int8_t idx = g_animationStack[slot];
        if (idx < 0 || idx >= (int8_t)G_NUM_ANIMATIONS) continue;

        Animation* a = g_animations[idx];
        if (a->active())
            a->apply(state, fb, timeMs);
    }
}
