#include <unity.h>
#include <cstring>
#include "framebuffer.h"
#include "animation.h"
#include "animations/rotation.h"

static Framebuffer fb;

void setUp() {
    fb = Framebuffer();
    TEST_ASSERT_TRUE(fb.init(36, 10));
}

void tearDown() {
    fb.release();
    for (uint8_t i = 0; i < G_NUM_ANIMATIONS; i++)
        g_animations[i]->resetDefaults();
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

void test_rotation_has_one_param() {
    RotationAnimation rot;
    TEST_ASSERT_EQUAL_UINT8(1, rot.paramCount());
    TEST_ASSERT_EQUAL_STRING("speed", rot.param(0).key);
    TEST_ASSERT_EQUAL_STRING("Speed", rot.param(0).label);
}

void test_rotation_presets() {
    RotationAnimation rot;
    const Param& p = rot.param(0);
    TEST_ASSERT_EQUAL_UINT8(4, p.optionCount);
    TEST_ASSERT_EQUAL_STRING("Off", p.options[0].label);
    TEST_ASSERT_EQUAL_INT32(0, p.options[0].value);
    TEST_ASSERT_EQUAL_STRING("Fast", p.options[3].label);
    TEST_ASSERT_EQUAL_INT32(90, p.options[3].value);
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
    TEST_ASSERT_EQUAL_INT32(42, rot.param(0).value);
    rot.resetDefaults();
    TEST_ASSERT_EQUAL_INT32(0, rot.param(0).value);
}

// ---------- Global registry ----------

void test_registry_has_rotation() {
    TEST_ASSERT_GREATER_THAN(0, G_NUM_ANIMATIONS);
    TEST_ASSERT_EQUAL_STRING("rot", g_animations[0]->key());
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

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_rotation_name);
    RUN_TEST(test_rotation_key);
    RUN_TEST(test_rotation_has_one_param);
    RUN_TEST(test_rotation_presets);

    RUN_TEST(test_rotation_inactive_at_zero);
    RUN_TEST(test_rotation_active_when_nonzero);

    RUN_TEST(test_rotation_zero_offset_at_time_zero);
    RUN_TEST(test_rotation_produces_offset_over_time);
    RUN_TEST(test_rotation_offset_wraps_360);
    RUN_TEST(test_rotation_higher_speed_more_offset);

    RUN_TEST(test_find_param_exists);
    RUN_TEST(test_find_param_missing);
    RUN_TEST(test_reset_defaults);

    RUN_TEST(test_registry_has_rotation);

    RUN_TEST(test_apply_skips_inactive);
    RUN_TEST(test_apply_runs_active);

    return UNITY_END();
}
