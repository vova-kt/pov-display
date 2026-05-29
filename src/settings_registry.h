#pragma once
#include <cstdint>
#include "param.h"
#include <ArduinoJson.h>

struct Config;

enum class Scope : uint8_t {
    Both    = 0,
    McuOnly = 1,
    SimOnly = 2,
};

struct Setting {
    const char* key;
    const char* label;
    const char* group;            // "picture" | "hardware"
    const char* section;          // display section within the group
    Scope       scope;
    ParamType   type;
    int32_t     defaultVal;
    int32_t     min;              // Int only
    int32_t     max;              // Int only
    uint8_t     scale;            // 0 or 1 = no scale; >1 = display value / scale
    const ParamOption* options;
    uint8_t     optionCount;

    int32_t  (*getInt)();
    void     (*setInt)(int32_t);
    const char* (*getText)();
    void     (*setText)(const char*);

    const char* nvsKey;           // non-null => persisted
};

extern const Setting g_settings[];
extern const uint16_t G_NUM_SETTINGS;

extern const Setting g_sim_settings[];
extern const uint16_t G_NUM_SIM_SETTINGS;

namespace settings_registry {
    void init(Config* cfg);
    Config* config();

    void loadFromNvs();
    void saveToNvs();
    void clearNvs();
    void resetToDefaults();

    // Filter: Scope::McuOnly emits Both + McuOnly entries; Scope::SimOnly emits Both + SimOnly.
    bool entryVisible(const Setting& s, Scope side);

    void toJson(JsonObject root, Scope side);
    void applyJson(JsonObjectConst patch, Scope side);
}
