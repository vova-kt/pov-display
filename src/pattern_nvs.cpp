#include "patterns/registry.h"
#include "param.h"
#include <Preferences.h>
#include <cstdio>

static void patNvsKey(char* buf, size_t bufSz, const char* patKey, const char* paramKey) {
    snprintf(buf, bufSz, "p_%s_%s", patKey, paramKey);
}

void loadPatternsFromNvs() {
    Preferences prefs;
    if (!prefs.begin("pov", true)) return;
    char nvsKey[24];
    for (uint8_t i = 0; i < G_NUM_PATTERNS; i++) {
        Pattern* p = g_patterns[i];
        for (uint8_t j = 0; j < p->paramCount(); j++) {
            Param& par = p->param(j);
            patNvsKey(nvsKey, sizeof(nvsKey), p->key(), par.key);
            if (par.type == ParamType::Text) {
                if (par.textBuf && par.textBufSize > 0)
                    prefs.getString(nvsKey, par.textBuf, par.textBufSize);
            } else {
                par.value = prefs.getInt(nvsKey, par.defaultVal);
            }
        }
    }
    prefs.end();
}

void savePatternsToNvs() {
    Preferences prefs;
    if (!prefs.begin("pov", false)) return;
    char nvsKey[24];
    for (uint8_t i = 0; i < G_NUM_PATTERNS; i++) {
        Pattern* p = g_patterns[i];
        for (uint8_t j = 0; j < p->paramCount(); j++) {
            const Param& par = p->param(j);
            patNvsKey(nvsKey, sizeof(nvsKey), p->key(), par.key);
            if (par.type == ParamType::Text) {
                if (par.textBuf) prefs.putString(nvsKey, par.textBuf);
            } else {
                prefs.putInt(nvsKey, par.value);
            }
        }
    }
    prefs.end();
}
