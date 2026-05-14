#include <unity.h>
#include "output_scale.h"

static constexpr uint16_t NUM_LEDS = 26;
static uint8_t scale[MAX_LEDS];

void setUp() {}
void tearDown() {}

// --- radialBalance = false: no scaling ---

void test_disabled_all_255() {
    bool active = compute_output_scale(scale, MAX_LEDS, NUM_LEDS, false);
    TEST_ASSERT_FALSE(active);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        TEST_ASSERT_EQUAL_UINT8(255, scale[i]);
    }
}

// --- radialBalance = true: basic properties ---

void test_enabled_outer_led_full() {
    compute_output_scale(scale, MAX_LEDS, NUM_LEDS, true);
    TEST_ASSERT_EQUAL_UINT8(255, scale[NUM_LEDS - 1]);
}

void test_enabled_monotonically_increasing() {
    compute_output_scale(scale, MAX_LEDS, NUM_LEDS, true);
    for (uint16_t i = 1; i < NUM_LEDS; i++) {
        TEST_ASSERT_GREATER_OR_EQUAL(scale[i - 1], scale[i]);
    }
}

void test_enabled_returns_active() {
    bool active = compute_output_scale(scale, MAX_LEDS, NUM_LEDS, true);
    TEST_ASSERT_TRUE(active);
}

// --- overcompensation guard ---
// Perceived brightness = physical_boost × correction.
// Physical boost is capped at MAX_RADIAL_BOOST (3×), so the correction
// floor must be 1/MAX_RADIAL_BOOST to avoid over-dimming inner LEDs.

void test_no_overcompensation() {
    compute_output_scale(scale, MAX_LEDS, NUM_LEDS, true);
    uint8_t floor = (uint8_t)(255.0f / MAX_RADIAL_BOOST + 0.5f);  // 85
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        TEST_ASSERT_GREATER_OR_EQUAL(floor, scale[i]);
    }
}

// --- unused LEDs beyond numLeds stay at 255 ---

void test_unused_leds_full() {
    compute_output_scale(scale, MAX_LEDS, NUM_LEDS, true);
    for (uint16_t i = NUM_LEDS; i < MAX_LEDS; i++) {
        TEST_ASSERT_EQUAL_UINT8(255, scale[i]);
    }
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_disabled_all_255);
    RUN_TEST(test_enabled_outer_led_full);
    RUN_TEST(test_enabled_monotonically_increasing);
    RUN_TEST(test_enabled_returns_active);
    RUN_TEST(test_no_overcompensation);
    RUN_TEST(test_unused_leds_full);
    return UNITY_END();
}
