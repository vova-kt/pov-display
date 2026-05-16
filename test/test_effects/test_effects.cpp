#include <unity.h>
#include <cstring>
#include "framebuffer.h"
#include "effect.h"
#include "effects/bloom.h"
#include "effects/fisheye_scale.h"
#include "effects/rotation.h"
#include "effects/scale.h"
#include "../../sim/effect_phase.h"

static Framebuffer fb;

static Effect* effectByKey(const char* key) {
    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++)
        if (strcmp(g_effects[i]->key(), key) == 0) return g_effects[i];
    return nullptr;
}

void setUp() {
    fb = Framebuffer();
    TEST_ASSERT_TRUE(fb.init(36, 10));
    resetEffectStackDefaults();
}

void tearDown() {
    fb.release();
    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++)
        g_effects[i]->resetDefaults();
    resetEffectStackDefaults();
}

// ---------- RotationEffect metadata ----------

void test_rotation_name() {
    RotationEffect rot;
    TEST_ASSERT_EQUAL_STRING("Rotation", rot.name());
}

void test_rotation_key() {
    RotationEffect rot;
    TEST_ASSERT_EQUAL_STRING("rot", rot.key());
}

void test_rotation_has_params() {
    RotationEffect rot;
    TEST_ASSERT_EQUAL_UINT8(2, rot.paramCount());
    TEST_ASSERT_EQUAL_STRING("speed", rot.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Speed", rot.param(0).label);
    TEST_ASSERT_EQUAL_STRING("direction", rot.param(1).key);
    TEST_ASSERT_EQUAL_STRING("Direction", rot.param(1).label);
}

void test_rotation_speed_presets() {
    RotationEffect rot;
    const Param& p = rot.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("Off", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(0, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("Fast", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(90, p.options[3].value);
}

void test_rotation_direction_default_clockwise() {
    RotationEffect rot;
    const Param& p = rot.param(1);
    TEST_ASSERT_EQUAL_INT32(1, p.defaultVal);
    TEST_ASSERT_EQUAL_INT32(1, p.value);
}

void test_rotation_direction_presets() {
    RotationEffect rot;
    const Param& p = rot.param(1);
    TEST_ASSERT_EQUAL_UINT8(2, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("Clockwise", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(1, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("Counterclockwise", p.options[1].label);
    TEST_ASSERT_EQUAL_INT32(-1, p.options[1].value);
}

// ---------- RotationEffect active/inactive ----------

void test_rotation_inactive_at_zero() {
    RotationEffect rot;
    TEST_ASSERT_FALSE(rot.active());
}

void test_rotation_active_when_nonzero() {
    RotationEffect rot;
    rot.param(0).value = 45;
    TEST_ASSERT_TRUE(rot.active());
}

// ---------- RotationEffect::apply ----------

void test_rotation_zero_offset_at_time_zero() {
    RotationEffect rot;
    rot.param(0).value = 45;
    EffectState state;
    rot.apply(state, fb, 0);
    TEST_ASSERT_EQUAL_INT16(0, state.sliceOffset);
}

void test_rotation_produces_offset_over_time() {
    RotationEffect rot;
    rot.param(0).value = 90;
    EffectState state;
    rot.apply(state, fb, 1000);
    TEST_ASSERT_NOT_EQUAL(0, state.sliceOffset);
}

void test_rotation_offset_wraps_360() {
    RotationEffect rot;
    rot.param(0).value = 90;
    EffectState state;
    rot.apply(state, fb, 10000);
    TEST_ASSERT_TRUE(state.sliceOffset >= -360 && state.sliceOffset <= 360);
}

void test_rotation_higher_speed_more_offset() {
    RotationEffect rot;

    rot.param(0).value = 15;
    EffectState slow;
    rot.apply(slow, fb, 2000);

    rot.param(0).value = 90;
    EffectState fast;
    rot.apply(fast, fb, 2000);

    int16_t slowAbs = slow.sliceOffset < 0 ? -slow.sliceOffset : slow.sliceOffset;
    int16_t fastAbs = fast.sliceOffset < 0 ? -fast.sliceOffset : fast.sliceOffset;
    TEST_ASSERT_GREATER_THAN(slowAbs, fastAbs);
}

void test_rotation_clockwise_positive_offset() {
    RotationEffect rot;
    rot.param(0).value = 90;
    rot.param(1).value = 1;
    EffectState state;
    rot.apply(state, fb, 1000);
    TEST_ASSERT_GREATER_THAN(0, state.sliceOffset);
}

void test_rotation_counterclockwise_negative_offset() {
    RotationEffect rot;
    rot.param(0).value = 90;
    rot.param(1).value = -1;
    EffectState state;
    rot.apply(state, fb, 1000);
    TEST_ASSERT_LESS_THAN(0, state.sliceOffset);
}

// ---------- ScaleEffect metadata ----------

void test_scale_name() {
    ScaleEffect scale;
    TEST_ASSERT_EQUAL_STRING("Scale", scale.name());
}

void test_scale_key() {
    ScaleEffect scale;
    TEST_ASSERT_EQUAL_STRING("scale", scale.key());
}

void test_scale_has_params() {
    ScaleEffect scale;
    TEST_ASSERT_EQUAL_UINT8(2, scale.paramCount());
    TEST_ASSERT_EQUAL_STRING("duration", scale.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Duration", scale.param(0).label);
    TEST_ASSERT_EQUAL_STRING("factor", scale.param(1).key);
    TEST_ASSERT_EQUAL_STRING("Scale factor", scale.param(1).label);
}

void test_scale_duration_presets() {
    ScaleEffect scale;
    const Param& p = scale.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("200 ms", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(200, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("2000 ms", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(2000, p.options[3].value);
}

void test_scale_factor_presets() {
    ScaleEffect scale;
    const Param& p = scale.param(1);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("1.2", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(12, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("3.0", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(30, p.options[3].value);
}

// ---------- ScaleEffect::apply ----------

void test_scale_noop_at_cycle_start() {
    ScaleEffect scale;
    scale.param(0).value = 1000;
    scale.param(1).value = 20;
    fb.setPixel(0, 6, 7, 0, 0, 31);

    EffectState state;
    scale.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(7, slice[6].red);
}

void test_scale_expands_inner_leds_at_peak() {
    ScaleEffect scale;
    scale.param(0).value = 1000;
    scale.param(1).value = 20;
    for (uint16_t led = 0; led < fb.numLeds(); led++)
        fb.setPixel(0, led, led + 1, 0, 0, 31);

    EffectState state;
    scale.apply(state, fb, 500);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(1, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(1, slice[1].red);
    TEST_ASSERT_EQUAL_UINT8(2, slice[2].red);
    TEST_ASSERT_EQUAL_UINT8(3, slice[5].red);
}

// ---------- FisheyeScaleEffect metadata ----------

void test_fisheye_scale_name() {
    FisheyeScaleEffect scale;
    TEST_ASSERT_EQUAL_STRING("Fisheye Scale", scale.name());
}

void test_fisheye_scale_key() {
    FisheyeScaleEffect scale;
    TEST_ASSERT_EQUAL_STRING("fisheye", scale.key());
}

void test_fisheye_scale_has_params() {
    FisheyeScaleEffect scale;
    TEST_ASSERT_EQUAL_UINT8(2, scale.paramCount());
    TEST_ASSERT_EQUAL_STRING("duration", scale.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Duration", scale.param(0).label);
    TEST_ASSERT_EQUAL_STRING("factor", scale.param(1).key);
    TEST_ASSERT_EQUAL_STRING("Scale factor", scale.param(1).label);
}

void test_fisheye_scale_duration_presets() {
    FisheyeScaleEffect scale;
    const Param& p = scale.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("200 ms", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(200, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("2000 ms", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(2000, p.options[3].value);
}

void test_fisheye_scale_factor_presets() {
    FisheyeScaleEffect scale;
    const Param& p = scale.param(1);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("1.2", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(12, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("3.0", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(30, p.options[3].value);
}

// ---------- FisheyeScaleEffect::apply ----------

void test_fisheye_scale_noop_at_cycle_start() {
    FisheyeScaleEffect scale;
    scale.param(0).value = 1000;
    scale.param(1).value = 30;
    for (uint16_t led = 0; led < fb.numLeds(); led++)
        fb.setPixel(0, led, led + 1, 0, 0, 31);

    EffectState state;
    scale.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(1, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(6, slice[5].red);
    TEST_ASSERT_EQUAL_UINT8(10, slice[9].red);
}

void test_fisheye_scale_expands_center_and_anchors_edge() {
    FisheyeScaleEffect scale;
    scale.param(0).value = 1000;
    scale.param(1).value = 30;
    for (uint16_t led = 0; led < fb.numLeds(); led++)
        fb.setPixel(0, led, led + 1, 0, 0, 31);

    EffectState state;
    scale.apply(state, fb, 500);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(1, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(1, slice[1].red);
    TEST_ASSERT_EQUAL_UINT8(2, slice[2].red);
    TEST_ASSERT_EQUAL_UINT8(5, slice[5].red);
    TEST_ASSERT_EQUAL_UINT8(10, slice[9].red);
}

// ---------- Effect base class ----------

void test_find_param_exists() {
    RotationEffect rot;
    Param* p = rot.findParam("speed");
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_STRING("speed", p->key);
}

void test_find_param_missing() {
    RotationEffect rot;
    Param* p = rot.findParam("nonexistent");
    TEST_ASSERT_NULL(p);
}

void test_reset_defaults() {
    RotationEffect rot;
    rot.param(0).value = 42;
    rot.param(1).value = -1;
    TEST_ASSERT_EQUAL_INT32(42, rot.param(0).value);
    TEST_ASSERT_EQUAL_INT32(-1, rot.param(1).value);
    rot.resetDefaults();
    TEST_ASSERT_EQUAL_INT32(0, rot.param(0).value);
    TEST_ASSERT_EQUAL_INT32(1, rot.param(1).value);
}

// ---------- Global registry ----------

void test_registry_has_rotation() {
    TEST_ASSERT_GREATER_THAN(0, G_NUM_EFFECTS);
    TEST_ASSERT_NOT_NULL(effectByKey("rot"));
}

void test_registry_has_scale() {
    TEST_ASSERT_NOT_NULL(effectByKey("scale"));
}

void test_registry_has_fisheye_scale() {
    TEST_ASSERT_NOT_NULL(effectByKey("fisheye"));
}

// ---------- Effect stack ----------

void test_effect_stack_defaults_to_all_empty() {
    for (uint8_t i = 0; i < MAX_EFFECT_SLOTS; i++)
        TEST_ASSERT_EQUAL_STRING("", effectSlotKey(i));
}

void test_set_effect_slot_changes_order() {
    TEST_ASSERT_TRUE(setEffectSlot(0, "scale"));
    TEST_ASSERT_TRUE(setEffectSlot(1, "rot"));
    TEST_ASSERT_EQUAL_STRING("scale", effectSlotKey(0));
    TEST_ASSERT_EQUAL_STRING("rot", effectSlotKey(1));
}

void test_set_effect_slot_rejects_unknown_key() {
    TEST_ASSERT_TRUE(setEffectSlot(0, "scale"));
    TEST_ASSERT_FALSE(setEffectSlot(0, "missing"));
    TEST_ASSERT_EQUAL_STRING("scale", effectSlotKey(0));
}

void test_apply_uses_stack_selection() {
    Effect* rot = effectByKey("rot");
    TEST_ASSERT_NOT_NULL(rot);
    rot->findParam("speed")->value = 90;

    TEST_ASSERT_TRUE(setEffectSlot(0, "scale"));
    TEST_ASSERT_TRUE(setEffectSlot(1, ""));
    EffectState scaleOnly;
    applyEffects(scaleOnly, fb, 1000);
    TEST_ASSERT_EQUAL_INT16(0, scaleOnly.sliceOffset);

    TEST_ASSERT_TRUE(setEffectSlot(0, "rot"));
    EffectState rotationOnly;
    applyEffects(rotationOnly, fb, 1000);
    TEST_ASSERT_NOT_EQUAL(0, rotationOnly.sliceOffset);
}

void test_apply_runs_later_stack_slot() {
    Effect* rot = effectByKey("rot");
    TEST_ASSERT_NOT_NULL(rot);
    rot->findParam("speed")->value = 45;

    TEST_ASSERT_TRUE(setEffectSlot(0, ""));
    TEST_ASSERT_TRUE(setEffectSlot(1, ""));
    TEST_ASSERT_TRUE(setEffectSlot(2, "rot"));

    EffectState state;
    applyEffects(state, fb, 2000);
    TEST_ASSERT_NOT_EQUAL(0, state.sliceOffset);
}

// ---------- applyEffects ----------

void test_apply_skips_inactive() {
    setEffectSlot(0, "rot");
    Effect* rot = effectByKey("rot");
    rot->param(0).value = 0;
    EffectState state;
    applyEffects(state, fb, 5000);
    TEST_ASSERT_EQUAL_INT16(0, state.sliceOffset);
}

void test_apply_runs_active() {
    setEffectSlot(0, "rot");
    Effect* rot = effectByKey("rot");
    rot->param(0).value = 45;
    EffectState state;
    applyEffects(state, fb, 2000);
    TEST_ASSERT_NOT_EQUAL(0, state.sliceOffset);
}

void test_effect_nvs_roundtrip_stack_and_scale_param() {
    Effect* scale = effectByKey("scale");
    TEST_ASSERT_NOT_NULL(scale);

    TEST_ASSERT_TRUE(setEffectSlot(0, "scale"));
    TEST_ASSERT_TRUE(setEffectSlot(1, "rot"));
    scale->findParam("factor")->value = 30;
    saveEffectsToNvs();

    resetEffectStackDefaults();
    for (uint8_t i = 0; i < G_NUM_EFFECTS; i++)
        g_effects[i]->resetDefaults();

    loadEffectsFromNvs();

    TEST_ASSERT_EQUAL_STRING("scale", effectSlotKey(0));
    TEST_ASSERT_EQUAL_STRING("rot", effectSlotKey(1));
    TEST_ASSERT_EQUAL_INT32(30, scale->findParam("factor")->value);
}

// ---------- BloomEffect metadata ----------

void test_bloom_name() {
    BloomEffect bloom;
    TEST_ASSERT_EQUAL_STRING("Bloom", bloom.name());
}

void test_bloom_key() {
    BloomEffect bloom;
    TEST_ASSERT_EQUAL_STRING("bloom", bloom.key());
}

void test_bloom_has_params() {
    BloomEffect bloom;
    TEST_ASSERT_EQUAL_UINT8(3, bloom.paramCount());
    TEST_ASSERT_EQUAL_STRING("radius", bloom.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Radius", bloom.param(0).label);
    TEST_ASSERT_EQUAL_STRING("intensity", bloom.param(1).key);
    TEST_ASSERT_EQUAL_STRING("Intensity", bloom.param(1).label);
    TEST_ASSERT_EQUAL_STRING("threshold", bloom.param(2).key);
    TEST_ASSERT_EQUAL_STRING("Threshold", bloom.param(2).label);
}

void test_bloom_radius_presets() {
    BloomEffect bloom;
    const Param& p = bloom.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("1 LED", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(1, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("4 LEDs", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(4, p.options[3].value);
}

void test_bloom_intensity_presets() {
    BloomEffect bloom;
    const Param& p = bloom.param(1);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("25%", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(64, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("100%", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(255, p.options[3].value);
}

void test_bloom_threshold_presets() {
    BloomEffect bloom;
    const Param& p = bloom.param(2);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("Off", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(0, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("High", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(192, p.options[3].value);
}

void test_bloom_defaults() {
    BloomEffect bloom;
    TEST_ASSERT_EQUAL_INT32(2, bloom.param(0).value);
    TEST_ASSERT_EQUAL_INT32(128, bloom.param(1).value);
    TEST_ASSERT_EQUAL_INT32(128, bloom.param(2).value);
}

// ---------- BloomEffect::apply ----------

void test_bloom_noop_dark_frame() {
    BloomEffect bloom;
    bloom.param(2).value = 128;
    fb.clearBack();

    fb.setPixel(0, 3, 50, 50, 50, 31);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(50, slice[3].red);
    TEST_ASSERT_EQUAL_UINT8(0, slice[2].red);
    TEST_ASSERT_EQUAL_UINT8(0, slice[4].red);
}

void test_bloom_spreads_bright_pixel() {
    BloomEffect bloom;
    bloom.param(0).value = 2;
    bloom.param(1).value = 255;
    bloom.param(2).value = 0;
    fb.clearBack();

    fb.setPixel(0, 5, 200, 0, 0, 31);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_GREATER_THAN(0, slice[4].red);
    TEST_ASSERT_GREATER_THAN(0, slice[6].red);
    TEST_ASSERT_GREATER_THAN(0, slice[3].red);
    TEST_ASSERT_EQUAL_UINT8(0, slice[2].red);
}

void test_bloom_respects_threshold() {
    BloomEffect bloom;
    bloom.param(0).value = 1;
    bloom.param(1).value = 255;
    bloom.param(2).value = 128;
    fb.clearBack();

    fb.setPixel(0, 5, 100, 0, 0, 31);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0, slice[4].red);
    TEST_ASSERT_EQUAL_UINT8(0, slice[6].red);
}

void test_bloom_above_threshold_spreads() {
    BloomEffect bloom;
    bloom.param(0).value = 1;
    bloom.param(1).value = 255;
    bloom.param(2).value = 128;
    fb.clearBack();

    fb.setPixel(0, 5, 200, 0, 0, 31);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_GREATER_THAN(0, slice[4].red);
    TEST_ASSERT_GREATER_THAN(0, slice[6].red);
}

void test_bloom_clamps_to_255() {
    BloomEffect bloom;
    bloom.param(0).value = 1;
    bloom.param(1).value = 255;
    bloom.param(2).value = 0;
    fb.clearBack();

    fb.setPixel(0, 5, 255, 255, 255, 31);
    fb.setPixel(0, 6, 255, 255, 255, 31);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, slice[5].red);
    TEST_ASSERT_EQUAL_UINT8(255, slice[6].red);
}

void test_bloom_linear_falloff() {
    BloomEffect bloom;
    bloom.param(0).value = 2;
    bloom.param(1).value = 255;
    bloom.param(2).value = 0;
    fb.clearBack();

    fb.setPixel(0, 5, 200, 0, 0, 31);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_GREATER_THAN(slice[3].red, slice[4].red);
}

void test_bloom_propagates_brightness() {
    BloomEffect bloom;
    bloom.param(0).value = 1;
    bloom.param(1).value = 255;
    bloom.param(2).value = 0;
    fb.clearBack();

    fb.setPixel(0, 5, 200, 0, 0, 20);

    EffectState state;
    bloom.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(20, slice[4].brightness & 0x1F);
    TEST_ASSERT_EQUAL_UINT8(20, slice[6].brightness & 0x1F);
}

void test_bloom_registered_in_registry() {
    TEST_ASSERT_NOT_NULL(effectByKey("bloom"));
}

// ---------- Simulator effect phase ----------

void test_sim_phase_keeps_last_offset_between_generation_frames() {
    SimEffectPhase phase;
    EffectState generated;
    generated.sliceOffset = 27;

    phase.update(generated);

    TEST_ASSERT_EQUAL_INT16(37, phase.phaseOffset(10));
    TEST_ASSERT_EQUAL_INT16(37, phase.phaseOffset(10));
}

void test_sim_phase_reset_returns_to_base_phase() {
    SimEffectPhase phase;
    EffectState generated;
    generated.sliceOffset = -45;
    phase.update(generated);

    phase.reset();

    TEST_ASSERT_EQUAL_INT16(90, phase.phaseOffset(90));
    TEST_ASSERT_EQUAL_INT16(0, phase.sliceOffset());
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_rotation_name);
    RUN_TEST(test_rotation_key);
    RUN_TEST(test_rotation_has_params);
    RUN_TEST(test_rotation_speed_presets);
    RUN_TEST(test_rotation_direction_default_clockwise);
    RUN_TEST(test_rotation_direction_presets);

    RUN_TEST(test_rotation_inactive_at_zero);
    RUN_TEST(test_rotation_active_when_nonzero);

    RUN_TEST(test_rotation_zero_offset_at_time_zero);
    RUN_TEST(test_rotation_produces_offset_over_time);
    RUN_TEST(test_rotation_offset_wraps_360);
    RUN_TEST(test_rotation_higher_speed_more_offset);
    RUN_TEST(test_rotation_clockwise_positive_offset);
    RUN_TEST(test_rotation_counterclockwise_negative_offset);

    RUN_TEST(test_scale_name);
    RUN_TEST(test_scale_key);
    RUN_TEST(test_scale_has_params);
    RUN_TEST(test_scale_duration_presets);
    RUN_TEST(test_scale_factor_presets);
    RUN_TEST(test_scale_noop_at_cycle_start);
    RUN_TEST(test_scale_expands_inner_leds_at_peak);

    RUN_TEST(test_fisheye_scale_name);
    RUN_TEST(test_fisheye_scale_key);
    RUN_TEST(test_fisheye_scale_has_params);
    RUN_TEST(test_fisheye_scale_duration_presets);
    RUN_TEST(test_fisheye_scale_factor_presets);
    RUN_TEST(test_fisheye_scale_noop_at_cycle_start);
    RUN_TEST(test_fisheye_scale_expands_center_and_anchors_edge);

    RUN_TEST(test_bloom_name);
    RUN_TEST(test_bloom_key);
    RUN_TEST(test_bloom_has_params);
    RUN_TEST(test_bloom_radius_presets);
    RUN_TEST(test_bloom_intensity_presets);
    RUN_TEST(test_bloom_threshold_presets);
    RUN_TEST(test_bloom_defaults);
    RUN_TEST(test_bloom_noop_dark_frame);
    RUN_TEST(test_bloom_spreads_bright_pixel);
    RUN_TEST(test_bloom_respects_threshold);
    RUN_TEST(test_bloom_above_threshold_spreads);
    RUN_TEST(test_bloom_clamps_to_255);
    RUN_TEST(test_bloom_linear_falloff);
    RUN_TEST(test_bloom_propagates_brightness);
    RUN_TEST(test_bloom_registered_in_registry);

    RUN_TEST(test_find_param_exists);
    RUN_TEST(test_find_param_missing);
    RUN_TEST(test_reset_defaults);

    RUN_TEST(test_registry_has_rotation);
    RUN_TEST(test_registry_has_scale);
    RUN_TEST(test_registry_has_fisheye_scale);

    RUN_TEST(test_effect_stack_defaults_to_all_empty);
    RUN_TEST(test_set_effect_slot_changes_order);
    RUN_TEST(test_set_effect_slot_rejects_unknown_key);
    RUN_TEST(test_apply_uses_stack_selection);
    RUN_TEST(test_apply_runs_later_stack_slot);

    RUN_TEST(test_apply_skips_inactive);
    RUN_TEST(test_apply_runs_active);
    RUN_TEST(test_effect_nvs_roundtrip_stack_and_scale_param);
    RUN_TEST(test_sim_phase_keeps_last_offset_between_generation_frames);
    RUN_TEST(test_sim_phase_reset_returns_to_base_phase);

    return UNITY_END();
}
