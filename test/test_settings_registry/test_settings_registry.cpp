#include <unity.h>
#include <cstring>
#include <ArduinoJson.h>
#include "config.h"
#include "settings_registry.h"
#include "patterns/registry.h"
#include "animation.h"

static Config cfg;

void setUp() {
    cfg = Config();
    settings_registry::init(&cfg);
    // Reset animations to defaults.
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++) g_animations[i]->resetDefaults();
}

void tearDown() {}

// ── toJson round-trip ──────────────────────────────────────────────────────

void test_tojson_contains_groups() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(doc["groups"].is<JsonArray>());
    TEST_ASSERT_EQUAL_INT(2, doc["groups"].as<JsonArray>().size());
}

void test_tojson_picture_has_brightness() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    for (JsonObject g : doc["groups"].as<JsonArray>()) {
        if (strcmp(g["key"].as<const char*>(), "picture") == 0) {
            for (JsonObject s : g["settings"].as<JsonArray>()) {
                if (strcmp(s["key"].as<const char*>(), "brightness") == 0) {
                    TEST_ASSERT_EQUAL_INT(cfg.brightness, s["value"].as<int>());
                    return;
                }
            }
        }
    }
    TEST_FAIL_MESSAGE("brightness setting not found");
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
    for (JsonObject g : doc["groups"].as<JsonArray>()) {
        for (JsonObject s : g["settings"].as<JsonArray>()) {
            if (strcmp(s["key"].as<const char*>(), "color") == 0) {
                TEST_ASSERT_EQUAL_INT(0x123456, s["value"].as<int>());
                return;
            }
        }
    }
    TEST_FAIL_MESSAGE("color setting not found");
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

void test_tojson_includes_animations() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    TEST_ASSERT_TRUE(doc["animations"].is<JsonArray>());
    TEST_ASSERT_GREATER_THAN(0, doc["animations"].as<JsonArray>().size());
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

void test_apply_rejects_invalid_numArms_enum() {
    uint8_t prev = cfg.numArms;
    JsonDocument doc;
    doc["settings"]["numArms"] = 3;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(prev, cfg.numArms);
}

void test_apply_accepts_valid_numArms_enum() {
    JsonDocument doc;
    doc["settings"]["numArms"] = 4;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_UINT8(4, cfg.numArms);
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

// ── applyJson animation params ─────────────────────────────────────────────

void test_apply_animation_param() {
    JsonDocument doc;
    doc["animations"]["rot"]["speed"] = 45;
    doc["animations"]["rot"]["direction"] = -1;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    TEST_ASSERT_EQUAL_INT32(45, g_animations[0]->findParam("speed")->value);
    TEST_ASSERT_EQUAL_INT32(-1, g_animations[0]->findParam("direction")->value);
}

// ── Scope filter ───────────────────────────────────────────────────────────

void test_scope_mcu_no_sim_settings() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::McuOnly);
    // SimOnly keys (from g_sim_settings) — stub is empty so no SimOnly entries
    // exist anyway. Confirm escPulseUs IS present (McuOnly).
    bool found = false;
    for (JsonObject g : doc["groups"].as<JsonArray>())
        for (JsonObject s : g["settings"].as<JsonArray>())
            if (strcmp(s["key"].as<const char*>(), "escPulseUs") == 0) found = true;
    TEST_ASSERT_TRUE_MESSAGE(found, "escPulseUs should appear in McuOnly model");
}

void test_scope_sim_no_mcu_only() {
    JsonDocument doc;
    settings_registry::toJson(doc.to<JsonObject>(), Scope::SimOnly);
    bool found = false;
    for (JsonObject g : doc["groups"].as<JsonArray>())
        for (JsonObject s : g["settings"].as<JsonArray>())
            if (strcmp(s["key"].as<const char*>(), "escPulseUs") == 0) found = true;
    TEST_ASSERT_FALSE_MESSAGE(found, "escPulseUs should NOT appear in SimOnly model");
}

// ── NVS round-trip ────────────────────────────────────────────────────────

void test_nvs_roundtrip() {
    cfg.brightness = 22;
    cfg.numArms = 4;
    settings_registry::saveToNvs();
    cfg.brightness = 0;
    cfg.numArms = 1;
    settings_registry::loadFromNvs();
    TEST_ASSERT_EQUAL_UINT8(22, cfg.brightness);
    TEST_ASSERT_EQUAL_UINT8(4, cfg.numArms);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_tojson_contains_groups);
    RUN_TEST(test_tojson_picture_has_brightness);
    RUN_TEST(test_tojson_activepattern_toplevel);
    RUN_TEST(test_tojson_color_packed);
    RUN_TEST(test_tojson_includes_patterns);
    RUN_TEST(test_tojson_text_pattern_has_params);
    RUN_TEST(test_tojson_includes_animations);

    RUN_TEST(test_apply_sets_brightness);
    RUN_TEST(test_apply_clamps_brightness_to_max);
    RUN_TEST(test_apply_rejects_invalid_numArms_enum);
    RUN_TEST(test_apply_accepts_valid_numArms_enum);
    RUN_TEST(test_apply_ignores_unknown_key);
    RUN_TEST(test_apply_color_unpacks_rgb);
    RUN_TEST(test_apply_mirror_bool);
    RUN_TEST(test_apply_activePattern_toplevel);
    RUN_TEST(test_apply_activePattern_out_of_range_rejected);

    RUN_TEST(test_apply_text_param);
    RUN_TEST(test_apply_text_mode_param);
    RUN_TEST(test_apply_text_margin_param);
    RUN_TEST(test_apply_text_truncates_long_string);
    RUN_TEST(test_apply_animation_param);

    RUN_TEST(test_scope_mcu_no_sim_settings);
    RUN_TEST(test_scope_sim_no_mcu_only);

    RUN_TEST(test_nvs_roundtrip);

    return UNITY_END();
}
