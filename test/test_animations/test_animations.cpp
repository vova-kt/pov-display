#include <unity.h>
#include <cstring>
#include "framebuffer.h"
#include "animation.h"
#include "animations/rotation.h"
#include "animations/scale.h"
#include "../../sim/animation_phase.h"

static Framebuffer fb;

static Animation* animationByKey(const char* key) {
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++)
        if (strcmp(g_animations[i]->key(), key) == 0) return g_animations[i];
    return nullptr;
}

void setUp() {
    fb = Framebuffer();
    TEST_ASSERT_TRUE(fb.init(36, 10));
    resetAnimationStackDefaults();
}

void tearDown() {
    fb.release();
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++)
        g_animations[i]->resetDefaults();
    resetAnimationStackDefaults();
}

// ---------- RotationAnimation metadata ----------

void test_rotation_name() {
    RotationAnimation rot;
    TEST_ASSERT_EQUAL_STRING("Rotation", rot.name());
}

void test_rotation_key() {
    RotationAnimation rot;
    TEST_ASSERT_EQUAL_STRING("rot", rot.key());
}

void test_rotation_has_params() {
    RotationAnimation rot;
    TEST_ASSERT_EQUAL_UINT8(2, rot.paramCount());
    TEST_ASSERT_EQUAL_STRING("speed", rot.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Speed", rot.param(0).label);
    TEST_ASSERT_EQUAL_STRING("direction", rot.param(1).key);
    TEST_ASSERT_EQUAL_STRING("Direction", rot.param(1).label);
}

void test_rotation_speed_presets() {
    RotationAnimation rot;
    const Param& p = rot.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("Off", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(0, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("Fast", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(90, p.options[3].value);
}

void test_rotation_direction_default_clockwise() {
    RotationAnimation rot;
    const Param& p = rot.param(1);
    TEST_ASSERT_EQUAL_INT32(1, p.defaultVal);
    TEST_ASSERT_EQUAL_INT32(1, p.value);
}

void test_rotation_direction_presets() {
    RotationAnimation rot;
    const Param& p = rot.param(1);
    TEST_ASSERT_EQUAL_UINT8(2, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("Clockwise", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(1, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("Counterclockwise", p.options[1].label);
    TEST_ASSERT_EQUAL_INT32(-1, p.options[1].value);
}

// ---------- RotationAnimation active/inactive ----------

void test_rotation_inactive_at_zero() {
    RotationAnimation rot;
    TEST_ASSERT_FALSE(rot.active());
}

void test_rotation_active_when_nonzero() {
    RotationAnimation rot;
    rot.param(0).value = 45;
    TEST_ASSERT_TRUE(rot.active());
}

// ---------- RotationAnimation::apply ----------

void test_rotation_zero_offset_at_time_zero() {
    RotationAnimation rot;
    rot.param(0).value = 45;
    AnimationState state;
    rot.apply(state, fb, 0);
    TEST_ASSERT_EQUAL_INT16(0, state.sliceOffset);
}

void test_rotation_produces_offset_over_time() {
    RotationAnimation rot;
    rot.param(0).value = 90;
    AnimationState state;
    rot.apply(state, fb, 1000);
    TEST_ASSERT_NOT_EQUAL(0, state.sliceOffset);
}

void test_rotation_offset_wraps_360() {
    RotationAnimation rot;
    rot.param(0).value = 90;
    AnimationState state;
    rot.apply(state, fb, 10000);
    TEST_ASSERT_TRUE(state.sliceOffset >= -360 && state.sliceOffset <= 360);
}

void test_rotation_higher_speed_more_offset() {
    RotationAnimation rot;

    rot.param(0).value = 15;
    AnimationState slow;
    rot.apply(slow, fb, 2000);

    rot.param(0).value = 90;
    AnimationState fast;
    rot.apply(fast, fb, 2000);

    int16_t slowAbs = slow.sliceOffset < 0 ? -slow.sliceOffset : slow.sliceOffset;
    int16_t fastAbs = fast.sliceOffset < 0 ? -fast.sliceOffset : fast.sliceOffset;
    TEST_ASSERT_GREATER_THAN(slowAbs, fastAbs);
}

void test_rotation_clockwise_positive_offset() {
    RotationAnimation rot;
    rot.param(0).value = 90;
    rot.param(1).value = 1;
    AnimationState state;
    rot.apply(state, fb, 1000);
    TEST_ASSERT_GREATER_THAN(0, state.sliceOffset);
}

void test_rotation_counterclockwise_negative_offset() {
    RotationAnimation rot;
    rot.param(0).value = 90;
    rot.param(1).value = -1;
    AnimationState state;
    rot.apply(state, fb, 1000);
    TEST_ASSERT_LESS_THAN(0, state.sliceOffset);
}

// ---------- ScaleAnimation metadata ----------

void test_scale_name() {
    ScaleAnimation scale;
    TEST_ASSERT_EQUAL_STRING("Scale", scale.name());
}

void test_scale_key() {
    ScaleAnimation scale;
    TEST_ASSERT_EQUAL_STRING("scale", scale.key());
}

void test_scale_has_params() {
    ScaleAnimation scale;
    TEST_ASSERT_EQUAL_UINT8(2, scale.paramCount());
    TEST_ASSERT_EQUAL_STRING("duration", scale.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Duration", scale.param(0).label);
    TEST_ASSERT_EQUAL_STRING("factor", scale.param(1).key);
    TEST_ASSERT_EQUAL_STRING("Scale factor", scale.param(1).label);
}

void test_scale_duration_presets() {
    ScaleAnimation scale;
    const Param& p = scale.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("200 ms", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(200, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("2000 ms", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(2000, p.options[3].value);
}

void test_scale_factor_presets() {
    ScaleAnimation scale;
    const Param& p = scale.param(1);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("1.2", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(12, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("3.0", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(30, p.options[3].value);
}

// ---------- ScaleAnimation::apply ----------

void test_scale_noop_at_cycle_start() {
    ScaleAnimation scale;
    scale.param(0).value = 1000;
    scale.param(1).value = 20;
    fb.setPixel(0, 6, 7, 0, 0, 31);

    AnimationState state;
    scale.apply(state, fb, 0);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(7, slice[6].red);
}

void test_scale_expands_inner_leds_at_peak() {
    ScaleAnimation scale;
    scale.param(0).value = 1000;
    scale.param(1).value = 20;
    for (uint16_t led = 0; led < fb.numLeds(); led++)
        fb.setPixel(0, led, led + 1, 0, 0, 31);

    AnimationState state;
    scale.apply(state, fb, 500);

    Pixel* slice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(1, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(1, slice[1].red);
    TEST_ASSERT_EQUAL_UINT8(2, slice[2].red);
    TEST_ASSERT_EQUAL_UINT8(3, slice[5].red);
}

// ---------- Animation base class ----------

void test_find_param_exists() {
    RotationAnimation rot;
    Param* p = rot.findParam("speed");
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_STRING("speed", p->key);
}

void test_find_param_missing() {
    RotationAnimation rot;
    Param* p = rot.findParam("nonexistent");
    TEST_ASSERT_NULL(p);
}

void test_reset_defaults() {
    RotationAnimation rot;
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
    TEST_ASSERT_GREATER_THAN(0, G_NUM_ANIMATIONS);
    TEST_ASSERT_EQUAL_STRING("rot", g_animations[0]->key());
}

void test_registry_has_scale() {
    TEST_ASSERT_NOT_NULL(animationByKey("scale"));
}

// ---------- Animation stack ----------

void test_animation_stack_defaults_to_rotation_first() {
    TEST_ASSERT_EQUAL_STRING("rot", animationSlotKey(0));
    TEST_ASSERT_EQUAL_STRING("", animationSlotKey(1));
}

void test_set_animation_slot_changes_order() {
    TEST_ASSERT_TRUE(setAnimationSlot(0, "scale"));
    TEST_ASSERT_TRUE(setAnimationSlot(1, "rot"));
    TEST_ASSERT_EQUAL_STRING("scale", animationSlotKey(0));
    TEST_ASSERT_EQUAL_STRING("rot", animationSlotKey(1));
}

void test_set_animation_slot_rejects_unknown_key() {
    TEST_ASSERT_TRUE(setAnimationSlot(0, "scale"));
    TEST_ASSERT_FALSE(setAnimationSlot(0, "missing"));
    TEST_ASSERT_EQUAL_STRING("scale", animationSlotKey(0));
}

void test_apply_uses_stack_selection() {
    Animation* rot = animationByKey("rot");
    TEST_ASSERT_NOT_NULL(rot);
    rot->findParam("speed")->value = 90;

    TEST_ASSERT_TRUE(setAnimationSlot(0, "scale"));
    TEST_ASSERT_TRUE(setAnimationSlot(1, ""));
    AnimationState scaleOnly;
    applyAnimations(scaleOnly, fb, 1000);
    TEST_ASSERT_EQUAL_INT16(0, scaleOnly.sliceOffset);

    TEST_ASSERT_TRUE(setAnimationSlot(0, "rot"));
    AnimationState rotationOnly;
    applyAnimations(rotationOnly, fb, 1000);
    TEST_ASSERT_NOT_EQUAL(0, rotationOnly.sliceOffset);
}

void test_apply_runs_second_stack_slot() {
    Animation* rot = animationByKey("rot");
    TEST_ASSERT_NOT_NULL(rot);
    rot->findParam("speed")->value = 45;

    TEST_ASSERT_TRUE(setAnimationSlot(0, ""));
    TEST_ASSERT_TRUE(setAnimationSlot(1, "rot"));

    AnimationState state;
    applyAnimations(state, fb, 2000);
    TEST_ASSERT_NOT_EQUAL(0, state.sliceOffset);
}

// ---------- applyAnimations ----------

void test_apply_skips_inactive() {
    g_animations[0]->param(0).value = 0;
    AnimationState state;
    applyAnimations(state, fb, 5000);
    TEST_ASSERT_EQUAL_INT16(0, state.sliceOffset);
}

void test_apply_runs_active() {
    g_animations[0]->param(0).value = 45;
    AnimationState state;
    applyAnimations(state, fb, 2000);
    TEST_ASSERT_NOT_EQUAL(0, state.sliceOffset);
}

void test_animation_nvs_roundtrip_stack_and_scale_param() {
    Animation* scale = animationByKey("scale");
    TEST_ASSERT_NOT_NULL(scale);

    TEST_ASSERT_TRUE(setAnimationSlot(0, "scale"));
    TEST_ASSERT_TRUE(setAnimationSlot(1, "rot"));
    scale->findParam("factor")->value = 30;
    saveAnimationsToNvs();

    resetAnimationStackDefaults();
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++)
        g_animations[i]->resetDefaults();

    loadAnimationsFromNvs();

    TEST_ASSERT_EQUAL_STRING("scale", animationSlotKey(0));
    TEST_ASSERT_EQUAL_STRING("rot", animationSlotKey(1));
    TEST_ASSERT_EQUAL_INT32(30, scale->findParam("factor")->value);
}

// ---------- Simulator animation phase ----------

void test_sim_phase_keeps_last_offset_between_generation_frames() {
    SimAnimationPhase phase;
    AnimationState generated;
    generated.sliceOffset = 27;

    phase.update(generated);

    TEST_ASSERT_EQUAL_INT16(37, phase.phaseOffset(10));
    TEST_ASSERT_EQUAL_INT16(37, phase.phaseOffset(10));
}

void test_sim_phase_reset_returns_to_base_phase() {
    SimAnimationPhase phase;
    AnimationState generated;
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

    RUN_TEST(test_find_param_exists);
    RUN_TEST(test_find_param_missing);
    RUN_TEST(test_reset_defaults);

    RUN_TEST(test_registry_has_rotation);
    RUN_TEST(test_registry_has_scale);

    RUN_TEST(test_animation_stack_defaults_to_rotation_first);
    RUN_TEST(test_set_animation_slot_changes_order);
    RUN_TEST(test_set_animation_slot_rejects_unknown_key);
    RUN_TEST(test_apply_uses_stack_selection);
    RUN_TEST(test_apply_runs_second_stack_slot);

    RUN_TEST(test_apply_skips_inactive);
    RUN_TEST(test_apply_runs_active);
    RUN_TEST(test_animation_nvs_roundtrip_stack_and_scale_param);
    RUN_TEST(test_sim_phase_keeps_last_offset_between_generation_frames);
    RUN_TEST(test_sim_phase_reset_returns_to_base_phase);

    return UNITY_END();
}
