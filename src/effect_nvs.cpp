#include "effect.h"
#include <Preferences.h>
#include <cstdio>

static void effectNvsKey(char* buf, size_t bufSz, const char* effectKey, const char* paramKey) {
    snprintf(buf, bufSz, "fx_%s_%s", effectKey, paramKey);
}

static void effectStackNvsKey(char* buf, size_t bufSz, uint8_t slot) {
    snprintf(buf, bufSz, "fx_s%u", slot);
}

void loadEffectsFromNvs() {
    resetEffectStackDefaults();

    Preferences prefs;
    if (!prefs.begin("pov", true)) return;
    char nvsKey[20];
    char fxKey[16];

    for (uint8_t slot = 0; slot < MAX_EFFECT_SLOTS; slot++) {
        effectStackNvsKey(nvsKey, sizeof(nvsKey), slot);
        if (prefs.getString(nvsKey, fxKey, sizeof(fxKey)) > 0)
            setEffectSlot(slot, fxKey);
    }

    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++) {
        Effect* e = g_effects[i];
        for (uint8_t j = 0; j < e->paramCount(); j++) {
            Param& p = e->param(j);
            effectNvsKey(nvsKey, sizeof(nvsKey), e->key(), p.key);
            p.value = prefs.getInt(nvsKey, p.defaultVal);
        }
    }
    prefs.end();
}

void saveEffectsToNvs() {
    Preferences prefs;
    if (!prefs.begin("pov", false)) return;
    char nvsKey[20];

    for (uint8_t slot = 0; slot < MAX_EFFECT_SLOTS; slot++) {
        effectStackNvsKey(nvsKey, sizeof(nvsKey), slot);
        prefs.putString(nvsKey, effectSlotKey(slot));
    }

    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++) {
        Effect* e = g_effects[i];
        for (uint8_t j = 0; j < e->paramCount(); j++) {
            const Param& p = e->param(j);
            effectNvsKey(nvsKey, sizeof(nvsKey), e->key(), p.key);
            prefs.putInt(nvsKey, p.value);
        }
    }
    prefs.end();
}
