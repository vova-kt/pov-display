#include "effect.h"
#include "effects/rotation.h"
#include "effects/scale.h"
#include "effects/fisheye_scale.h"

static RotationEffect s_rotation;
static ScaleEffect s_scale;
static FisheyeScaleEffect s_fisheyeScale;

Effect* const g_effects[] = { &s_rotation, &s_scale, &s_fisheyeScale };
const uint8_t G_NUM_EFFECTS = sizeof(g_effects) / sizeof(g_effects[0]);
int8_t g_effectStack[MAX_EFFECT_SLOTS] = {-1, -1, -1, -1};

int8_t effectIndexForKey(const char* key) {
    if (!key || key[0] == '\0') return -1;
    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++) {
        if (strcmp(g_effects[i]->key(), key) == 0) return (int8_t)i;
    }
    return -1;
}

const char* effectSlotKey(uint8_t slot) {
    if (slot >= MAX_EFFECT_SLOTS) return "";
    int8_t idx = g_effectStack[slot];
    if (idx < 0 || idx >= (int8_t)G_NUM_EFFECTS) return "";
    return g_effects[idx]->key();
}

bool setEffectSlot(uint8_t slot, const char* key) {
    if (slot >= MAX_EFFECT_SLOTS) return false;
    if (!key || key[0] == '\0' || strcmp(key, "none") == 0) {
        g_effectStack[slot] = -1;
        return true;
    }

    int8_t idx = effectIndexForKey(key);
    if (idx < 0) return false;

    g_effectStack[slot] = idx;
    return true;
}

void resetEffectStackDefaults() {
    for (uint8_t i = 0; i < MAX_EFFECT_SLOTS; i++)
        g_effectStack[i] = -1;
}

void applyEffects(EffectState& state, Framebuffer& fb, uint32_t timeMs) {
    for (uint8_t slot = 0; slot < MAX_EFFECT_SLOTS; slot++) {
        int8_t idx = g_effectStack[slot];
        if (idx < 0 || idx >= (int8_t)G_NUM_EFFECTS) continue;

        Effect* e = g_effects[idx];
        if (e->active())
            e->apply(state, fb, timeMs);
    }
}
