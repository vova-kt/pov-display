#include <unity.h>
#include <cstring>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"
#include "settings_registry.h"
#include "patterns/registry.h"
#include "effect.h"

static Config cfg;

static JsonObject findGroup(JsonDocument& doc, const char* key) {
    for (JsonObject g : doc["groups"].as<JsonArray>())
        if (strcmp(g["key"].as<const char*>(), key) == 0) return g;
    return JsonObject();
}

static JsonObject findSection(JsonObject group, const char* key) {
    for (JsonObject section : group["sections"].as<JsonArray>())
        if (strcmp(section["key"].as<const char*>(), key) == 0) return section;
    return JsonObject();
}

static JsonObject findSetting(JsonDocument& doc, const char* key) {
    for (JsonObject g : doc["groups"].as<JsonArray>()) {
        for (JsonObject section : g["sections"].as<JsonArray>())
            for (JsonObject s : section["settings"].as<JsonArray>())
                if (strcmp(s["key"].as<const char*>(), key) == 0) return s;
    }
    return JsonObject();
}

static Effect* effectByKey(const char* key) {
    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++)
        if (strcmp(g_effects[i]->key(), key) == 0) return g_effects[i];
    return nullptr;
}

void setUp() {
    cfg = Config();
    settings_registry::init(&cfg);
    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++) g_effects[i]->resetDefaults();
    resetEffectStackDefaults();
}

void tearDown() {}

// ── toJson round-trip ──────────────────────────────────────────────────────

void test_tojson_contains_groups() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(doc["groups"].is<JsonArray>());
    TEST_ASSERT_EQUAL_INT(2, doc["groups"].as<JsonArray>().size());
}

void test_tojson_picture_has_ordered_sections() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);

    JsonObject picture = findGroup(doc, "picture");
    TEST_ASSERT_FALSE_MESSAGE(picture.isNull(), "picture group not found");
    JsonArray sections = picture["sections"].as<JsonArray>();
    TEST_ASSERT_EQUAL_INT(3, sections.size());
    TEST_ASSERT_EQUAL_STRING("pattern", sections[0]["key"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("effects", sections[1]["key"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("global", sections[2]["key"].as<const char*>());
}

void test_tojson_picture_has_brightness() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    JsonObject global = findSection(findGroup(doc, "picture"), "global");
    TEST_ASSERT_FALSE_MESSAGE(global.isNull(), "global section not found");
    for (JsonObject s : global["settings"].as<JsonArray>()) {
        if (strcmp(s["key"].as<const char*>(), "brightness") != 0) continue;
        TEST_ASSERT_EQUAL_INT(cfg.brightness, s["value"].as<int>());
        return;
    }
    TEST_FAIL_MESSAGE("brightness setting not found in global section");
}

void test_tojson_pattern_section_owns_pattern_setting() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    JsonObject pattern = findSection(findGroup(doc, "picture"), "pattern");
    TEST_ASSERT_FALSE_MESSAGE(pattern.isNull(), "pattern section not found");
    TEST_ASSERT_EQUAL_INT(1, pattern["settings"].as<JsonArray>().size());
    TEST_ASSERT_EQUAL_STRING("activePattern", pattern["settings"][0]["key"].as<const char*>());
}

void test_tojson_dynamic_params_have_sections() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);

    bool foundText = false;
    bool foundRot = false;
    for (JsonObject p : doc["patterns"].as<JsonArray>()) {
        if (strcmp(p["key"].as<const char*>(), "text") == 0) {
            TEST_ASSERT_EQUAL_STRING("picture", p["group"].as<const char*>());
            TEST_ASSERT_EQUAL_STRING("pattern", p["section"].as<const char*>());
            foundText = true;
        }
    }
    for (JsonObject e : doc["effects"].as<JsonArray>()) {
        if (strcmp(e["key"].as<const char*>(), "rot") == 0) {
            TEST_ASSERT_EQUAL_STRING("picture", e["group"].as<const char*>());
            TEST_ASSERT_EQUAL_STRING("effects", e["section"].as<const char*>());
            foundRot = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(foundText, "text pattern section not found");
    TEST_ASSERT_TRUE_MESSAGE(foundRot, "rotation effect section not found");
}

void test_tojson_activepattern_toplevel() {
    cfg.activePattern = 2;
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_INT(2, doc["activePattern"].as<int>());
}

void test_tojson_color_packed() {
    cfg.colorR = 0x12; cfg.colorG = 0x34; cfg.colorB = 0x56;
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    JsonObject s = findSetting(doc, "color");
    TEST_ASSERT_FALSE_MESSAGE(s.isNull(), "color setting not found");
    TEST_ASSERT_EQUAL_INT(0x123456, s["value"].as<int>());
}

void test_tojson_includes_patterns() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(doc["patterns"].is<JsonArray>());
    TEST_ASSERT_GREATER_THAN(0, doc["patterns"].as<JsonArray>().size());
}

void test_tojson_text_pattern_has_params() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    for (JsonObject p : doc["patterns"].as<JsonArray>()) {
        if (strcmp(p["key"].as<const char*>(), "text") == 0) {
            TEST_ASSERT_TRUE(p["params"].is<JsonArray>());
            TEST_ASSERT_EQUAL_INT(4, p["params"].as<JsonArray>().size());
            bool foundMargin = false;
            for (JsonObject param : p["params"].as<JsonArray>()) {
                if (strcmp(param["key"].as<const char*>(), "marginLeds") == 0) {
                    TEST_ASSERT_EQUAL_INT(1, param["value"].as<int>());
                    TEST_ASSERT_EQUAL_INT(1, param["default"].as<int>());
                    TEST_ASSERT_EQUAL_INT(0, param["min"].as<int>());
                    foundMargin = true;
                }
            }
            TEST_ASSERT_TRUE_MESSAGE(foundMargin, "Text pattern should expose marginLeds");
            return;
        }
    }
    TEST_FAIL_MESSAGE("text pattern not found");
}

void test_tojson_text_delay_is_fixed_enum() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    for (JsonObject p : doc["patterns"].as<JsonArray>()) {
        if (strcmp(p["key"].as<const char*>(), "text") != 0) continue;
        for (JsonObject param : p["params"].as<JsonArray>()) {
            if (strcmp(param["key"].as<const char*>(), "delayMs") != 0) continue;
            TEST_ASSERT_EQUAL_STRING("enum", param["type"].as<const char*>());
            JsonArray options = param["options"].as<JsonArray>();
            TEST_ASSERT_EQUAL_INT(6, options.size());
            const int expected[] = {50, 100, 250, 500, 1000, 2000};
            for (uint8_t i = 0; i < 6; i++)
                TEST_ASSERT_EQUAL_INT(expected[i], options[i][1].as<int>());
            return;
        }
    }
    TEST_FAIL_MESSAGE("text delayMs param not found");
}

void test_tojson_matrix_pattern_has_speed_param() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    for (JsonObject p : doc["patterns"].as<JsonArray>()) {
        if (strcmp(p["key"].as<const char*>(), "matrix") != 0) continue;
        TEST_ASSERT_EQUAL_STRING("matrix", p["name"].as<const char*>());
        TEST_ASSERT_TRUE(p["params"].is<JsonArray>());
        TEST_ASSERT_EQUAL_INT(1, p["params"].as<JsonArray>().size());

        JsonObject speed = p["params"][0].as<JsonObject>();
        TEST_ASSERT_EQUAL_STRING("speed", speed["key"].as<const char*>());
        TEST_ASSERT_EQUAL_STRING("int", speed["type"].as<const char*>());
        TEST_ASSERT_EQUAL_INT(12, speed["value"].as<int>());
        TEST_ASSERT_EQUAL_INT(12, speed["default"].as<int>());
        TEST_ASSERT_EQUAL_INT(1, speed["min"].as<int>());
        TEST_ASSERT_EQUAL_INT(48, speed["max"].as<int>());
        return;
    }
    TEST_FAIL_MESSAGE("matrix pattern not found");
}

void test_tojson_includes_effects() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(doc["effects"].is<JsonArray>());
    TEST_ASSERT_GREATER_THAN(0, doc["effects"].as<JsonArray>().size());
}

void test_tojson_includes_effect_stack() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(doc["effectStack"].is<JsonArray>());
    TEST_ASSERT_EQUAL_INT(MAX_EFFECT_SLOTS, doc["effectStack"].as<JsonArray>().size());
    for (uint8_t i = 0; i < MAX_EFFECT_SLOTS; i++)
        TEST_ASSERT_EQUAL_STRING("", doc["effectStack"][i].as<const char*>());
}

void test_tojson_includes_scale_effect() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    for (JsonObject e : doc["effects"].as<JsonArray>()) {
        if (strcmp(e["key"].as<const char*>(), "scale") == 0) {
            TEST_ASSERT_EQUAL_STRING("Scale", e["name"].as<const char*>());
            TEST_ASSERT_TRUE(e["params"].is<JsonArray>());
            TEST_ASSERT_EQUAL_INT(2, e["params"].as<JsonArray>().size());
            return;
        }
    }
    TEST_FAIL_MESSAGE("scale effect not found");
}

void test_tojson_includes_fisheye_scale_effect() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    for (JsonObject e : doc["effects"].as<JsonArray>()) {
        if (strcmp(e["key"].as<const char*>(), "fisheye") == 0) {
            TEST_ASSERT_EQUAL_STRING("Fisheye Scale", e["name"].as<const char*>());
            TEST_ASSERT_TRUE(e["params"].is<JsonArray>());
            TEST_ASSERT_EQUAL_INT(2, e["params"].as<JsonArray>().size());
            return;
        }
    }
    TEST_FAIL_MESSAGE("fisheye effect not found");
}

// ── applyJson settings ────────────────────────────────────────────────────

void test_apply_sets_brightness() {
    JsonDocument doc;
    doc["settings"]["brightness"] = 20;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(20, cfg.brightness);
}

void test_apply_clamps_brightness_to_max() {
    cfg.maxBrightness = 20;
    JsonDocument doc;
    doc["settings"]["brightness"] = 99;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(20, cfg.brightness);
}

void test_apply_numArms_ignored_on_mcu() {
    uint8_t prev = cfg.numArms;
    JsonDocument doc;
    doc["settings"]["numArms"] = 4;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(prev, cfg.numArms);
}

void test_apply_numSlices_ignored_by_settings() {
    uint16_t prev = cfg.numSlices;
    JsonDocument doc;
    doc["settings"]["numSlices"] = 720;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT16(prev, cfg.numSlices);
}

void test_apply_ignores_unknown_key() {
    uint8_t prev = cfg.brightness;
    JsonDocument doc;
    doc["settings"]["notarealkey"] = 42;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(prev, cfg.brightness);
}

void test_apply_color_unpacks_rgb() {
    JsonDocument doc;
    doc["settings"]["color"] = 0xFF8000;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(0xFF, cfg.colorR);
    TEST_ASSERT_EQUAL_UINT8(0x80, cfg.colorG);
    TEST_ASSERT_EQUAL_UINT8(0x00, cfg.colorB);
}

void test_apply_mirror_bool() {
    cfg.mirrorPattern = false;
    JsonDocument doc;
    doc["settings"]["mirrorPattern"] = 1;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(cfg.mirrorPattern);
}

void test_apply_activePattern_toplevel() {
    JsonDocument doc;
    doc["activePattern"] = 3;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(3, cfg.activePattern);
}

void test_apply_activePattern_out_of_range_rejected() {
    uint8_t prev = cfg.activePattern;
    JsonDocument doc;
    doc["activePattern"] = 99;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(prev, cfg.activePattern);
}

// ── applyJson pattern params ───────────────────────────────────────────────

void test_apply_text_param() {
    JsonDocument doc;
    doc["patterns"]["text"]["text"] = "HELLO";
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    int idx = g_pattern_index("text");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    Param* p = g_patterns[idx]->findParam("text");
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_STRING("HELLO", p->textBuf);
}

void test_apply_text_mode_param() {
    JsonDocument doc;
    doc["patterns"]["text"]["mode"] = 2;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    int idx = g_pattern_index("text");
    TEST_ASSERT_EQUAL_INT32(2, g_patterns[idx]->findParam("mode")->value);
}

void test_apply_text_margin_param() {
    JsonDocument doc;
    doc["patterns"]["text"]["marginLeds"] = 4;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    int idx = g_pattern_index("text");
    TEST_ASSERT_EQUAL_INT32(4, g_patterns[idx]->findParam("marginLeds")->value);
}

void test_apply_text_delay_accepts_only_fixed_values() {
    int idx = g_pattern_index("text");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    Param* delay = g_patterns[idx]->findParam("delayMs");
    TEST_ASSERT_NOT_NULL(delay);
    delay->value = 500;

    JsonDocument doc;
    doc["patterns"]["text"]["delayMs"] = 250;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_INT32(250, delay->value);

    doc.clear();
    doc["patterns"]["text"]["delayMs"] = 333;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_INT32(250, delay->value);
}

void test_apply_text_truncates_long_string() {
    char longStr[128];
    memset(longStr, 'A', 127);
    longStr[127] = '\0';
    JsonDocument doc;
    doc["patterns"]["text"]["text"] = longStr;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    int idx = g_pattern_index("text");
    Param* p = g_patterns[idx]->findParam("text");
    TEST_ASSERT_TRUE(strlen(p->textBuf) < p->textBufSize);
}

void test_apply_matrix_speed_clamps_to_range() {
    int idx = g_pattern_index("matrix");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    Param* speed = g_patterns[idx]->findParam("speed");
    TEST_ASSERT_NOT_NULL(speed);
    speed->value = 8;

    JsonDocument doc;
    doc["patterns"]["matrix"]["speed"] = 99;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_INT32(48, speed->value);

    doc.clear();
    doc["patterns"]["matrix"]["speed"] = -5;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_INT32(1, speed->value);
}

// ── applyJson effect params ────────────────────────────────────────────────

void test_apply_effect_param() {
    JsonDocument doc;
    doc["effects"]["rot"]["speed"] = 45;
    doc["effects"]["rot"]["direction"] = -1;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);

    Effect* rot = effectByKey("rot");
    TEST_ASSERT_NOT_NULL(rot);
    TEST_ASSERT_EQUAL_INT32(45, rot->findParam("speed")->value);
    TEST_ASSERT_EQUAL_INT32(-1, rot->findParam("direction")->value);
}

void test_apply_scale_effect_param() {
    JsonDocument doc;
    doc["effects"]["scale"]["duration"] = 1000;
    doc["effects"]["scale"]["factor"] = 20;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);

    Effect* scale = effectByKey("scale");
    TEST_ASSERT_NOT_NULL(scale);
    TEST_ASSERT_EQUAL_INT32(1000, scale->findParam("duration")->value);
    TEST_ASSERT_EQUAL_INT32(20, scale->findParam("factor")->value);
}

void test_apply_fisheye_scale_effect_param() {
    JsonDocument doc;
    doc["effects"]["fisheye"]["duration"] = 2000;
    doc["effects"]["fisheye"]["factor"] = 30;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);

    Effect* fisheye = effectByKey("fisheye");
    TEST_ASSERT_NOT_NULL(fisheye);
    TEST_ASSERT_EQUAL_INT32(2000, fisheye->findParam("duration")->value);
    TEST_ASSERT_EQUAL_INT32(30, fisheye->findParam("factor")->value);
}

void test_apply_effect_stack_order() {
    JsonDocument doc;
    doc["effectStack"][0] = "scale";
    doc["effectStack"][1] = "rot";
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);

    TEST_ASSERT_EQUAL_STRING("scale", effectSlotKey(0));
    TEST_ASSERT_EQUAL_STRING("rot", effectSlotKey(1));
}

void test_apply_effect_stack_rejects_unknown() {
    TEST_ASSERT_TRUE(setEffectSlot(0, "rot"));

    JsonDocument doc;
    doc["effectStack"][0] = "missing";
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);

    TEST_ASSERT_EQUAL_STRING("rot", effectSlotKey(0));
}

// ── Scope filter ───────────────────────────────────────────────────────────

void test_scope_removed_settings_absent() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE_MESSAGE(findSetting(doc, "escPulseUs").isNull(),
                             "escPulseUs removed from registry");
    TEST_ASSERT_TRUE_MESSAGE(findSetting(doc, "numArms").isNull(),
                             "numArms is now build-time constant, not a setting");
    TEST_ASSERT_TRUE_MESSAGE(findSetting(doc, "numSlices").isNull(),
                             "numSlices is a config field, not a setting");
    TEST_ASSERT_FALSE_MESSAGE(findSetting(doc, "targetHz").isNull(),
                              "targetHz should still appear");
}

// ── NVS round-trip ────────────────────────────────────────────────────────

void test_nvs_roundtrip() {
    cfg.brightness = 22;
    cfg.targetHz = 30;
    settings_registry::saveToNvs();
    cfg.brightness = 0;
    cfg.targetHz = 12;
    settings_registry::loadFromNvs();
    TEST_ASSERT_EQUAL_UINT8(22, cfg.brightness);
    TEST_ASSERT_EQUAL_UINT8(30, cfg.targetHz);
}

void test_pattern_nvs_rejects_invalid_text_delay() {
    int idx = g_pattern_index("text");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    Param* delay = g_patterns[idx]->findParam("delayMs");
    TEST_ASSERT_NOT_NULL(delay);
    delay->value = 500;

    Preferences prefs;
    TEST_ASSERT_TRUE(prefs.begin("pov", false));
    prefs.putInt("p_text_delayMs", 333);
    prefs.end();

    loadPatternsFromNvs();

    TEST_ASSERT_EQUAL_INT32(500, delay->value);
}

// ── Motor pulse derivation ────────────────────────────────────────────────

void test_targetHz_recomputes_escPulse() {
    cfg.motorStopped = false;
    cfg.numArms = 2;
    JsonDocument doc;
    doc["settings"]["targetHz"] = 30;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    // 30 Hz × 60 / 2 arms = 900 RPM → 1150 + 900*850/3600 = 1362
    TEST_ASSERT_EQUAL_UINT16(1362, cfg.escPulseUs);
}

void test_max_rpm_clamps_pulse() {
    cfg.motorStopped = false;
    cfg.numArms = 1;
    JsonDocument doc;
    doc["settings"]["targetHz"] = 60;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    // 60 Hz × 60 / 1 arm = 3600 RPM → 1150 + 850 = 2000 (max)
    TEST_ASSERT_EQUAL_UINT16(2000, cfg.escPulseUs);
}

void test_motor_stopped_prevents_recompute() {
    cfg.motorStopped = true;
    cfg.escPulseUs = kStopPulseUs;
    JsonDocument doc;
    doc["settings"]["targetHz"] = 60;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT16(kStopPulseUs, cfg.escPulseUs);
}

void test_clearNvs_removes_stored_settings() {
    cfg.brightness = 22;
    settings_registry::saveToNvs();
    settings_registry::clearNvs();
    cfg.brightness = 5;
    settings_registry::loadFromNvs();
    TEST_ASSERT_EQUAL_UINT8(5, cfg.brightness);
}

void test_resetToDefaults_restores_defaults() {
    cfg.brightness = 25;
    cfg.targetHz = 60;
    settings_registry::resetToDefaults();
    TEST_ASSERT_EQUAL_UINT8(16, cfg.brightness);
    TEST_ASSERT_EQUAL_UINT8(12, cfg.targetHz);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_tojson_contains_groups);
    RUN_TEST(test_tojson_picture_has_ordered_sections);
    RUN_TEST(test_tojson_picture_has_brightness);
    RUN_TEST(test_tojson_pattern_section_owns_pattern_setting);
    RUN_TEST(test_tojson_dynamic_params_have_sections);
    RUN_TEST(test_tojson_activepattern_toplevel);
    RUN_TEST(test_tojson_color_packed);
    RUN_TEST(test_tojson_includes_patterns);
    RUN_TEST(test_tojson_text_pattern_has_params);
    RUN_TEST(test_tojson_text_delay_is_fixed_enum);
    RUN_TEST(test_tojson_matrix_pattern_has_speed_param);
    RUN_TEST(test_tojson_includes_effects);
    RUN_TEST(test_tojson_includes_effect_stack);
    RUN_TEST(test_tojson_includes_scale_effect);
    RUN_TEST(test_tojson_includes_fisheye_scale_effect);

    RUN_TEST(test_apply_sets_brightness);
    RUN_TEST(test_apply_clamps_brightness_to_max);
    RUN_TEST(test_apply_numArms_ignored_on_mcu);
    RUN_TEST(test_apply_numSlices_ignored_by_settings);
    RUN_TEST(test_apply_ignores_unknown_key);
    RUN_TEST(test_apply_color_unpacks_rgb);
    RUN_TEST(test_apply_mirror_bool);
    RUN_TEST(test_apply_activePattern_toplevel);
    RUN_TEST(test_apply_activePattern_out_of_range_rejected);

    RUN_TEST(test_apply_text_param);
    RUN_TEST(test_apply_text_mode_param);
    RUN_TEST(test_apply_text_margin_param);
    RUN_TEST(test_apply_text_delay_accepts_only_fixed_values);
    RUN_TEST(test_apply_text_truncates_long_string);
    RUN_TEST(test_apply_matrix_speed_clamps_to_range);
    RUN_TEST(test_apply_effect_param);
    RUN_TEST(test_apply_scale_effect_param);
    RUN_TEST(test_apply_fisheye_scale_effect_param);
    RUN_TEST(test_apply_effect_stack_order);
    RUN_TEST(test_apply_effect_stack_rejects_unknown);

    RUN_TEST(test_scope_removed_settings_absent);

    RUN_TEST(test_nvs_roundtrip);
    RUN_TEST(test_pattern_nvs_rejects_invalid_text_delay);

    RUN_TEST(test_targetHz_recomputes_escPulse);
    RUN_TEST(test_max_rpm_clamps_pulse);
    RUN_TEST(test_motor_stopped_prevents_recompute);
    RUN_TEST(test_clearNvs_removes_stored_settings);
    RUN_TEST(test_resetToDefaults_restores_defaults);

    return UNITY_END();
}
