#include "settings_registry.h"
#include "config.h"
#include "animation.h"
#include "patterns/registry.h"
#include <Preferences.h>
#include <cstring>

static Config* s_cfg = nullptr;

namespace settings_registry {

void init(Config* cfg) { s_cfg = cfg; }
Config* config() { return s_cfg; }

} // namespace

// --- Accessors bound to Config fields ---

static int32_t  get_numLeds()       { return s_cfg->numLeds; }
static void     set_numLeds(int32_t v)       { s_cfg->numLeds = (uint16_t)v; }
static int32_t  get_numSlices()     { return s_cfg->numSlices; }
static void     set_numSlices(int32_t v)     { s_cfg->numSlices = (uint16_t)v; }
static int32_t  get_brightness()    { return s_cfg->brightness; }
static void     set_brightness(int32_t v)    {
    if (v > s_cfg->maxBrightness) v = s_cfg->maxBrightness;
    s_cfg->brightness = (uint8_t)v;
}
static int32_t  get_maxBrightness() { return s_cfg->maxBrightness; }
static void     set_maxBrightness(int32_t v) { s_cfg->maxBrightness = (uint8_t)v; }
static int32_t  get_phaseOffset()   { return s_cfg->phaseOffset; }
static void     set_phaseOffset(int32_t v)   { s_cfg->phaseOffset = (int16_t)v; }
static int32_t  get_activePattern() { return s_cfg->activePattern; }
static void     set_activePattern(int32_t v) {
    if (v >= 0 && v < G_NUM_PATTERNS) s_cfg->activePattern = (uint8_t)v;
}
static int32_t  get_color()         {
    return ((int32_t)s_cfg->colorR << 16) | ((int32_t)s_cfg->colorG << 8) | s_cfg->colorB;
}
static void     set_color(int32_t v) {
    s_cfg->colorR = (uint8_t)((v >> 16) & 0xFF);
    s_cfg->colorG = (uint8_t)((v >>  8) & 0xFF);
    s_cfg->colorB = (uint8_t)( v        & 0xFF);
}
static int32_t  get_numArms()       { return s_cfg->numArms; }
static void     set_numArms(int32_t v)       { s_cfg->numArms = (uint8_t)v; }
static int32_t  get_targetHz()      { return s_cfg->targetHz; }
static void     set_targetHz(int32_t v)      { s_cfg->targetHz = (uint8_t)v; }
static int32_t  get_escPulse()      { return s_cfg->escPulseUs; }
static void     set_escPulse(int32_t v)      { s_cfg->escPulseUs = (uint16_t)v; }
static int32_t  get_spiClock()      { return s_cfg->spiClockMhz; }
static void     set_spiClock(int32_t v)      { s_cfg->spiClockMhz = (uint8_t)v; }
static int32_t  get_mirror()        { return s_cfg->mirrorPattern ? 1 : 0; }
static void     set_mirror(int32_t v)        { s_cfg->mirrorPattern = v != 0; }
static int32_t  get_radialBalance() { return s_cfg->radialBalance ? 1 : 0; }
static void     set_radialBalance(int32_t v) { s_cfg->radialBalance = v != 0; }

// --- Enum option tables ---

static const ParamOption kArmOptions[]      = {{"1", 1}, {"2", 2}, {"4", 4}};
static const ParamOption kHzOptions[]       = {{"12", 12}, {"24", 24}, {"25", 25}, {"30", 30}, {"60", 60}};
static const ParamOption kSliceOptions[]    = {{"90", 90}, {"180", 180}, {"270", 270}, {"360", 360}};
static const ParamOption kSpiOptions[]      = {{"20", 20}, {"40", 40}};
static const ParamOption kPhaseOptions[]    = {{"0°", 0}, {"90°", 90}, {"180°", 180}, {"-90°", -90}};

// --- Registry table ---
// Picture group then hardware group; activePattern's enum options are synthesized at JSON time from g_patterns.

const Setting g_settings[] = {
    // ── Picture ────────────────────────────────────────────────────────────
    { "brightness",    "Brightness",     "picture", Scope::Both, ParamType::Int,
      16, 0, 31, 1, nullptr, 0,
      get_brightness, set_brightness, nullptr, nullptr, "brightness" },
    { "color",         "Color",          "picture", Scope::Both, ParamType::Color,
      0xFF0000, 0, 0xFFFFFF, 1, nullptr, 0,
      get_color, set_color, nullptr, nullptr, "color" },
    { "activePattern", "Pattern",        "picture", Scope::Both, ParamType::Enum,
      1, 0, 4, 1, nullptr, 0,    // options synthesized from g_patterns at JSON time
      get_activePattern, set_activePattern, nullptr, nullptr, "pattern" },
    { "mirrorPattern", "Mirror",         "picture", Scope::Both, ParamType::Bool,
      1, 0, 1, 1, nullptr, 0,
      get_mirror, set_mirror, nullptr, nullptr, "mirror" },
    { "radialBalance", "Radial balance", "picture", Scope::Both, ParamType::Bool,
      1, 0, 1, 1, nullptr, 0,
      get_radialBalance, set_radialBalance, nullptr, nullptr, "rad_bal" },
    { "phaseOffset",   "Phase offset",   "picture", Scope::Both, ParamType::Enum,
      0, -360, 360, 1, kPhaseOptions, 4,
      get_phaseOffset, set_phaseOffset, nullptr, nullptr, "phase_off" },

    // ── Hardware ───────────────────────────────────────────────────────────
    { "numArms",       "Arms",           "hardware", Scope::Both, ParamType::Enum,
      2, 1, 4, 1, kArmOptions, 3,
      get_numArms, set_numArms, nullptr, nullptr, "num_arms" },
    { "targetHz",      "Refresh rate",   "hardware", Scope::Both, ParamType::Enum,
      60, 0, 240, 1, kHzOptions, 5,
      get_targetHz, set_targetHz, nullptr, nullptr, "target_hz" },
    { "numLeds",       "LED count",      "hardware", Scope::Both, ParamType::Int,
      26, 1, MAX_LEDS, 1, nullptr, 0,
      get_numLeds, set_numLeds, nullptr, nullptr, "num_leds" },
    { "numSlices",     "Slice count",    "hardware", Scope::Both, ParamType::Enum,
      360, 0, 720, 1, kSliceOptions, 4,
      get_numSlices, set_numSlices, nullptr, nullptr, "num_slices" },
    { "escPulseUs",    "ESC pulse µs",   "hardware", Scope::McuOnly, ParamType::Int,
      1000, 1000, 2000, 1, nullptr, 0,
      get_escPulse, set_escPulse, nullptr, nullptr, "esc_pulse" },
    { "spiClockMhz",   "SPI clock MHz",  "hardware", Scope::Both, ParamType::Enum,
      20, 0, 40, 1, kSpiOptions, 2,
      get_spiClock, set_spiClock, nullptr, nullptr, "spi_clk" },
    { "maxBrightness", "Max brightness", "hardware", Scope::Both, ParamType::Int,
      31, 0, 31, 1, nullptr, 0,
      get_maxBrightness, set_maxBrightness, nullptr, nullptr, "max_bright" },
};
const uint16_t G_NUM_SETTINGS = sizeof(g_settings) / sizeof(g_settings[0]);

// --- JSON helpers ---

static const char* typeName(ParamType t) {
    switch (t) {
        case ParamType::Int:   return "int";
        case ParamType::Bool:  return "bool";
        case ParamType::Color: return "color";
        case ParamType::Text:  return "text";
        case ParamType::Enum:  return "enum";
    }
    return "int";
}

static void emitOptions(JsonObject obj, const ParamOption* opts, uint8_t count) {
    JsonArray arr = obj["options"].to<JsonArray>();
    for (uint8_t i = 0; i < count; i++) {
        JsonArray pair = arr.add<JsonArray>();
        pair.add(opts[i].label);
        pair.add(opts[i].value);
    }
}

static void emitParam(JsonObject obj, const Param& p) {
    obj["key"]     = p.key;
    obj["label"]   = p.label;
    obj["type"]    = typeName(p.type);
    if (p.type == ParamType::Text) {
        obj["value"]   = p.textBuf ? (const char*)p.textBuf : "";
        obj["default"] = "";
    } else {
        obj["value"]   = p.value;
        obj["default"] = p.defaultVal;
    }
    if (p.type == ParamType::Int) {
        obj["min"] = p.min;
        obj["max"] = p.max;
    }
    if (p.options && p.optionCount > 0)
        emitOptions(obj, p.options, p.optionCount);
}

static void emitSetting(JsonObject obj, const Setting& s) {
    obj["key"]     = s.key;
    obj["label"]   = s.label;
    obj["type"]    = typeName(s.type);
    if (s.type == ParamType::Text) {
        obj["value"]   = s.getText ? s.getText() : "";
        obj["default"] = "";
    } else {
        obj["value"]   = s.getInt ? s.getInt() : 0;
        obj["default"] = s.defaultVal;
    }
    if (s.type == ParamType::Int) {
        obj["min"] = s.min;
        obj["max"] = s.max;
    }
    if (s.scale > 1) obj["scale"] = s.scale;
    if (s.options && s.optionCount > 0)
        emitOptions(obj, s.options, s.optionCount);
    // Special case: activePattern options synthesized from g_patterns.
    if (strcmp(s.key, "activePattern") == 0) {
        JsonArray arr = obj["options"].to<JsonArray>();
        for (uint8_t i = 0; i < G_NUM_PATTERNS; i++) {
            JsonArray pair = arr.add<JsonArray>();
            pair.add(g_patterns[i]->name());
            pair.add(i);
        }
    }
}

namespace settings_registry {

bool entryVisible(const Setting& s, Scope side) {
    if (s.scope == Scope::Both) return true;
    return s.scope == side;
}

static void emitGroup(JsonObject groupObj, const char* group, const char* label, Scope side) {
    groupObj["key"]   = group;
    groupObj["label"] = label;
    JsonArray arr = groupObj["settings"].to<JsonArray>();
    for (uint16_t i = 0; i < G_NUM_SETTINGS; i++) {
        const Setting& s = g_settings[i];
        if (!entryVisible(s, side)) continue;
        if (strcmp(s.group, group) != 0) continue;
        emitSetting(arr.add<JsonObject>(), s);
    }
    for (uint16_t i = 0; i < G_NUM_SIM_SETTINGS; i++) {
        const Setting& s = g_sim_settings[i];
        if (!entryVisible(s, side)) continue;
        if (strcmp(s.group, group) != 0) continue;
        emitSetting(arr.add<JsonObject>(), s);
    }
}

void toJson(JsonObject root, Scope side) {
    root["activePattern"] = s_cfg ? s_cfg->activePattern : 0;

    JsonArray groups = root["groups"].to<JsonArray>();
    emitGroup(groups.add<JsonObject>(), "picture",  "Picture",  side);
    emitGroup(groups.add<JsonObject>(), "hardware", "Hardware", side);

    JsonArray patterns = root["patterns"].to<JsonArray>();
    for (uint8_t i = 0; i < G_NUM_PATTERNS; i++) {
        Pattern* p = g_patterns[i];
        JsonObject pObj = patterns.add<JsonObject>();
        pObj["key"]   = p->key();
        pObj["name"]  = p->name();
        pObj["index"] = i;
        pObj["group"] = "picture";
        JsonArray params = pObj["params"].to<JsonArray>();
        for (uint8_t j = 0; j < p->paramCount(); j++)
            emitParam(params.add<JsonObject>(), p->param(j));
    }

    JsonArray anims = root["animations"].to<JsonArray>();
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++) {
        Animation* a = g_animations[i];
        JsonObject aObj = anims.add<JsonObject>();
        aObj["key"]   = a->key();
        aObj["name"]  = a->name();
        aObj["group"] = "picture";
        JsonArray params = aObj["params"].to<JsonArray>();
        for (uint8_t j = 0; j < a->paramCount(); j++)
            emitParam(params.add<JsonObject>(), a->param(j));
    }
}

// --- Apply ---

static const Setting* findSetting(const char* key, Scope side) {
    for (uint16_t i = 0; i < G_NUM_SETTINGS; i++) {
        const Setting& s = g_settings[i];
        if (!entryVisible(s, side)) continue;
        if (strcmp(s.key, key) == 0) return &s;
    }
    for (uint16_t i = 0; i < G_NUM_SIM_SETTINGS; i++) {
        const Setting& s = g_sim_settings[i];
        if (!entryVisible(s, side)) continue;
        if (strcmp(s.key, key) == 0) return &s;
    }
    return nullptr;
}

static bool applySettingValue(const Setting& s, JsonVariantConst v) {
    if (s.type == ParamType::Text) {
        if (!v.is<const char*>()) return false;
        if (s.setText) s.setText(v.as<const char*>());
        return true;
    }
    if (!v.is<int32_t>() && !v.is<bool>()) return false;
    int32_t val = v.is<bool>() ? (v.as<bool>() ? 1 : 0) : v.as<int32_t>();
    switch (s.type) {
        case ParamType::Int:
            if (val < s.min) val = s.min;
            if (val > s.max) val = s.max;
            break;
        case ParamType::Bool:
            val = val ? 1 : 0;
            break;
        case ParamType::Color:
            val &= 0xFFFFFF;
            break;
        case ParamType::Enum:
            if (strcmp(s.key, "activePattern") == 0) {
                if (val < 0 || val >= G_NUM_PATTERNS) return false;
            } else {
                bool ok = false;
                for (uint8_t i = 0; i < s.optionCount; i++)
                    if (s.options[i].value == val) { ok = true; break; }
                if (!ok) return false;
            }
            break;
        case ParamType::Text:
            return false;
    }
    if (s.setInt) s.setInt(val);
    return true;
}

static void applyParamValue(Param& p, JsonVariantConst v) {
    if (p.type == ParamType::Text) {
        if (v.is<const char*>()) param_set_text(p, v.as<const char*>());
        return;
    }
    if (v.is<int32_t>()) param_set_int(p, v.as<int32_t>());
    else if (v.is<bool>()) param_set_int(p, v.as<bool>() ? 1 : 0);
}

void applyJson(JsonObjectConst patch, Scope side) {
    // settings: { key: value, ... }
    JsonObjectConst settings = patch["settings"].as<JsonObjectConst>();
    if (!settings.isNull()) {
        for (JsonPairConst kv : settings) {
            const Setting* s = findSetting(kv.key().c_str(), side);
            if (!s) continue;
            applySettingValue(*s, kv.value());
        }
    }

    // top-level activePattern shortcut
    if (patch["activePattern"].is<int32_t>()) {
        const Setting* s = findSetting("activePattern", side);
        if (s) applySettingValue(*s, patch["activePattern"]);
    }

    // patterns: { patternKey: { paramKey: value, ... }, ... }
    JsonObjectConst patterns = patch["patterns"].as<JsonObjectConst>();
    if (!patterns.isNull()) {
        for (JsonPairConst pkv : patterns) {
            int idx = g_pattern_index(pkv.key().c_str());
            if (idx < 0) continue;
            Pattern* p = g_patterns[idx];
            JsonObjectConst pObj = pkv.value().as<JsonObjectConst>();
            if (pObj.isNull()) continue;
            for (JsonPairConst kv : pObj) {
                Param* param = p->findParam(kv.key().c_str());
                if (!param) continue;
                applyParamValue(*param, kv.value());
            }
        }
    }

    // animations: { animKey: { paramKey: value, ... }, ... }
    JsonObjectConst animations = patch["animations"].as<JsonObjectConst>();
    if (!animations.isNull()) {
        for (JsonPairConst akv : animations) {
            Animation* a = nullptr;
            for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++)
                if (strcmp(g_animations[i]->key(), akv.key().c_str()) == 0) {
                    a = g_animations[i];
                    break;
                }
            if (!a) continue;
            JsonObjectConst aObj = akv.value().as<JsonObjectConst>();
            if (aObj.isNull()) continue;
            for (JsonPairConst kv : aObj) {
                Param* param = a->findParam(kv.key().c_str());
                if (!param) continue;
                applyParamValue(*param, kv.value());
            }
        }
    }
}

// --- NVS ---

static const char* NVS_NS = "pov";

void loadFromNvs() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return;
    for (uint16_t i = 0; i < G_NUM_SETTINGS; i++) {
        const Setting& s = g_settings[i];
        if (!s.nvsKey) continue;
        if (s.type == ParamType::Text) {
            if (!s.setText) continue;
            char buf[64];
            size_t n = prefs.getString(s.nvsKey, buf, sizeof(buf));
            if (n > 0) s.setText(buf);
        } else if (s.setInt) {
            int32_t v = prefs.getInt(s.nvsKey, s.getInt ? s.getInt() : s.defaultVal);
            s.setInt(v);
        }
    }
    prefs.end();
}

void saveToNvs() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return;
    for (uint16_t i = 0; i < G_NUM_SETTINGS; i++) {
        const Setting& s = g_settings[i];
        if (!s.nvsKey) continue;
        if (s.type == ParamType::Text) {
            if (s.getText) prefs.putString(s.nvsKey, s.getText());
        } else if (s.getInt) {
            prefs.putInt(s.nvsKey, s.getInt());
        }
    }
    prefs.end();
}

} // namespace settings_registry
