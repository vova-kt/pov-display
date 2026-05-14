#include <unity.h>
#include <cstring>
#include "framebuffer.h"
#include "config.h"
#include "patterns/solid.h"
#include "patterns/rainbow.h"
#include "patterns/scanner.h"
#include "patterns/text.h"

static Framebuffer fb;
static Config cfg;

void setUp() {
    fb = Framebuffer();
    TEST_ASSERT_TRUE(fb.init(36, 10));
    cfg = Config();
    cfg.numSlices = 36;
    cfg.numLeds = 10;
    cfg.brightness = 16;
    cfg.colorR = 255;
    cfg.colorG = 0;
    cfg.colorB = 0;
    strcpy(cfg.text, "HELLO");
}

void tearDown() {
    fb.release();
}

// ---------- Solid ----------

void test_solid_fills_all_pixels() {
    SolidPattern solid;
    solid.generate(fb, cfg, 0);

    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            TEST_ASSERT_EQUAL_UINT8(255, slice[l].red);
            TEST_ASSERT_EQUAL_UINT8(0, slice[l].green);
            TEST_ASSERT_EQUAL_UINT8(0, slice[l].blue);
            TEST_ASSERT_EQUAL_UINT8(0xE0 | 16, slice[l].brightness);
        }
    }
}

void test_solid_uses_config_color() {
    cfg.colorR = 10;
    cfg.colorG = 20;
    cfg.colorB = 30;
    cfg.brightness = 5;

    SolidPattern solid;
    solid.generate(fb, cfg, 0);

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(10, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(20, slice[0].green);
    TEST_ASSERT_EQUAL_UINT8(30, slice[0].blue);
    TEST_ASSERT_EQUAL_UINT8(0xE0 | 5, slice[0].brightness);
}

void test_solid_ignores_time() {
    SolidPattern solid;
    solid.generate(fb, cfg, 0);
    uint8_t r0 = fb.getSlice(0)[0].red;

    solid.generate(fb, cfg, 99999);
    TEST_ASSERT_EQUAL_UINT8(r0, fb.getSlice(0)[0].red);
}

void test_solid_name() {
    SolidPattern solid;
    TEST_ASSERT_EQUAL_STRING("solid", solid.name());
}

// ---------- Rainbow ----------

void test_rainbow_different_slices_different_hues() {
    RainbowPattern rainbow;
    rainbow.generate(fb, cfg, 0);

    const Pixel* s0 = fb.getSlice(0);
    const Pixel* s9 = fb.getSlice(9);
    bool different = (s0[0].red != s9[0].red) ||
                     (s0[0].green != s9[0].green) ||
                     (s0[0].blue != s9[0].blue);
    TEST_ASSERT_TRUE(different);
}

void test_rainbow_same_leds_within_slice() {
    RainbowPattern rainbow;
    rainbow.generate(fb, cfg, 0);

    const Pixel* slice = fb.getSlice(0);
    for (uint16_t l = 1; l < fb.numLeds(); l++) {
        TEST_ASSERT_EQUAL_UINT8(slice[0].red, slice[l].red);
        TEST_ASSERT_EQUAL_UINT8(slice[0].green, slice[l].green);
        TEST_ASSERT_EQUAL_UINT8(slice[0].blue, slice[l].blue);
    }
}

void test_rainbow_animates_over_time() {
    RainbowPattern rainbow;

    rainbow.generate(fb, cfg, 0);
    const Pixel* at0 = fb.getSlice(0);
    uint8_t r0 = at0[0].red, g0 = at0[0].green, b0 = at0[0].blue;

    rainbow.generate(fb, cfg, 1000);
    const Pixel* at1k = fb.getSlice(0);
    bool different = (r0 != at1k[0].red) ||
                     (g0 != at1k[0].green) ||
                     (b0 != at1k[0].blue);
    TEST_ASSERT_TRUE(different);
}

void test_rainbow_uses_config_brightness() {
    cfg.brightness = 20;
    RainbowPattern rainbow;
    rainbow.generate(fb, cfg, 0);

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0xE0 | 20, slice[0].brightness);
}

void test_rainbow_name() {
    RainbowPattern rainbow;
    TEST_ASSERT_EQUAL_STRING("rainbow", rainbow.name());
}

// ---------- Scanner ----------

void test_scanner_position_at_time_zero() {
    ScannerPattern scanner;
    scanner.generate(fb, cfg, 0);

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(0, slice[1].red);
}

void test_scanner_position_advances() {
    ScannerPattern scanner;
    // delay=32, pos = (32 / 32) % 10 = 1
    scanner.generate(fb, cfg, 32);

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(255, slice[1].red);
}

void test_scanner_wraps_around() {
    ScannerPattern scanner;
    // delay=32, pos = (320 / 32) % 10 = 0
    scanner.generate(fb, cfg, 320);

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, slice[0].red);
}

void test_scanner_only_one_led_lit() {
    ScannerPattern scanner;
    // delay=32, pos = (64 / 32) % 10 = 2
    scanner.generate(fb, cfg, 64);

    const Pixel* slice = fb.getSlice(0);
    int litCount = 0;
    for (uint16_t l = 0; l < fb.numLeds(); l++) {
        if (slice[l].red > 0 || slice[l].green > 0 || slice[l].blue > 0)
            litCount++;
    }
    TEST_ASSERT_EQUAL_INT(1, litCount);
}

void test_scanner_all_slices_same() {
    ScannerPattern scanner;
    // delay=32, pos = (64 / 32) % 10 = 2
    scanner.generate(fb, cfg, 64);

    const Pixel* s0 = fb.getSlice(0);
    const Pixel* s5 = fb.getSlice(5);
    TEST_ASSERT_EQUAL_UINT8(s0[2].red, s5[2].red);
    TEST_ASSERT_EQUAL_UINT8(s0[2].green, s5[2].green);
    TEST_ASSERT_EQUAL_UINT8(s0[2].blue, s5[2].blue);
}

void test_scanner_name() {
    ScannerPattern scanner;
    TEST_ASSERT_EQUAL_STRING("scanner", scanner.name());
}

// ---------- Text ----------

void test_text_empty_string_blank() {
    TextPattern text;
    strcpy(cfg.text, "");
    text.generate(fb, cfg, 0);

    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            TEST_ASSERT_EQUAL_UINT8(0, slice[l].brightness);
        }
    }
}

void test_text_renders_pixels() {
    TextPattern text;
    strcpy(cfg.text, "A");
    text.generate(fb, cfg, 0);

    int litCount = 0;
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            if (slice[l].brightness != 0) litCount++;
        }
    }
    TEST_ASSERT_GREATER_THAN(0, litCount);
}

void test_text_single_char_produces_lit_pixels() {
    TextPattern text;
    strcpy(cfg.text, "I");
    text.generate(fb, cfg, 0);

    int litCount = 0;
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            if (slice[l].brightness != 0) litCount++;
        }
    }
    TEST_ASSERT_GREATER_THAN(0, litCount);
}

void test_text_uses_config_color() {
    cfg.colorR = 10;
    cfg.colorG = 20;
    cfg.colorB = 30;
    strcpy(cfg.text, "A");

    TextPattern text;
    text.generate(fb, cfg, 0);

    bool found = false;
    for (uint16_t s = 0; s < fb.numSlices() && !found; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds() && !found; l++) {
            if (slice[l].brightness != 0) {
                TEST_ASSERT_EQUAL_UINT8(10, slice[l].red);
                TEST_ASSERT_EQUAL_UINT8(20, slice[l].green);
                TEST_ASSERT_EQUAL_UINT8(30, slice[l].blue);
                found = true;
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found, "No lit pixel found");
}

void test_text_centered_on_disc() {
    TextPattern text;
    strcpy(cfg.text, "H");
    text.generate(fb, cfg, 0);

    int litInner = 0;
    int litOuter = 0;
    uint16_t midLed = fb.numLeds() / 2;
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            if (slice[l].brightness != 0) {
                if (l < midLed) litInner++;
                else litOuter++;
            }
        }
    }
    TEST_ASSERT_GREATER_THAN(0, litInner);
    TEST_ASSERT_GREATER_THAN(0, litOuter);
}

void test_text_name() {
    TextPattern text;
    TEST_ASSERT_EQUAL_STRING("text", text.name());
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_solid_fills_all_pixels);
    RUN_TEST(test_solid_uses_config_color);
    RUN_TEST(test_solid_ignores_time);
    RUN_TEST(test_solid_name);

    RUN_TEST(test_rainbow_different_slices_different_hues);
    RUN_TEST(test_rainbow_same_leds_within_slice);
    RUN_TEST(test_rainbow_animates_over_time);
    RUN_TEST(test_rainbow_uses_config_brightness);
    RUN_TEST(test_rainbow_name);

    RUN_TEST(test_scanner_position_at_time_zero);
    RUN_TEST(test_scanner_position_advances);
    RUN_TEST(test_scanner_wraps_around);
    RUN_TEST(test_scanner_only_one_led_lit);
    RUN_TEST(test_scanner_all_slices_same);
    RUN_TEST(test_scanner_name);

    RUN_TEST(test_text_empty_string_blank);
    RUN_TEST(test_text_renders_pixels);
    RUN_TEST(test_text_single_char_produces_lit_pixels);
    RUN_TEST(test_text_uses_config_color);
    RUN_TEST(test_text_centered_on_disc);
    RUN_TEST(test_text_name);

    return UNITY_END();
}
