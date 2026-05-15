#include "animation.h"
#include <Preferences.h>
#include <cstdio>

static void animNvsKey(char* buf, size_t bufSz, const char* animKey, const char* paramKey) {
    snprintf(buf, bufSz, "a_%s_%s", animKey, paramKey);
}

void loadAnimationsFromNvs() {
    Preferences prefs;
    if (!prefs.begin("pov", true)) return;
    char nvsKey[20];
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++) {
        Animation* a = g_animations[i];
        for (uint8_t j = 0; j < a->paramCount(); j++) {
            Param& p = a->param(j);
            animNvsKey(nvsKey, sizeof(nvsKey), a->key(), p.key);
            p.value = prefs.getInt(nvsKey, p.defaultVal);
        }
    }
    prefs.end();
}

void saveAnimationsToNvs() {
    Preferences prefs;
    if (!prefs.begin("pov", false)) return;
    char nvsKey[20];
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++) {
        Animation* a = g_animations[i];
        for (uint8_t j = 0; j < a->paramCount(); j++) {
            const Param& p = a->param(j);
            animNvsKey(nvsKey, sizeof(nvsKey), a->key(), p.key);
            prefs.putInt(nvsKey, p.value);
        }
    }
    prefs.end();
}
