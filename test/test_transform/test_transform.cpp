#include <unity.h>
#include <cstring>
#include <cmath>
#include "canvas.h"
#include "framebuffer.h"
#include "config.h"
#include "transforms/polar_transform.h"
#include "transforms/identity_transform.h"

static Canvas canvas;
static Framebuffer fb;
static Config cfg;

void setUp() {
    canvas = Canvas();
    fb = Framebuffer();
    cfg = Config();
    cfg.numSlices = 36;
    cfg.numLeds = 10;
}

void tearDown() {
    canvas.release();
    fb.release();
}

// ---- Canvas tests ----

void test_canvas_init_sets_dimensions() {
    TEST_ASSERT_TRUE(canvas.init(20, 15));
    TEST_ASSERT_EQUAL_UINT16(20, canvas.width());
    TEST_ASSERT_EQUAL_UINT16(15, canvas.height());
}

void test_canvas_init_clears_buffer() {
    TEST_ASSERT_TRUE(canvas.init(10, 10));
    Pixel p = canvas.pixelAt(5, 5);
    TEST_ASSERT_EQUAL_UINT8(0, p.brightness);
    TEST_ASSERT_EQUAL_UINT8(0, p.red);
    TEST_ASSERT_EQUAL_UINT8(0, p.green);
    TEST_ASSERT_EQUAL_UINT8(0, p.blue);
}

void test_canvas_set_pixel_stores_rgba() {
    TEST_ASSERT_TRUE(canvas.init(10, 10));
    canvas.setPixel(3, 4, 100, 150, 200, 16);
    Pixel p = canvas.pixelAt(3, 4);
    TEST_ASSERT_EQUAL_UINT8(100, p.red);
    TEST_ASSERT_EQUAL_UINT8(150, p.green);
    TEST_ASSERT_EQUAL_UINT8(200, p.blue);
}

void test_canvas_brightness_encoding() {
    TEST_ASSERT_TRUE(canvas.init(10, 10));
    canvas.setPixel(0, 0, 255, 0, 0, 16);
    Pixel p = canvas.pixelAt(0, 0);
    TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix | 16, p.brightness);
}

void test_canvas_clear_zeros_pixels() {
    TEST_ASSERT_TRUE(canvas.init(10, 10));
    canvas.setPixel(3, 4, 255, 0, 0, 16);
    canvas.clear();
    Pixel p = canvas.pixelAt(3, 4);
    TEST_ASSERT_EQUAL_UINT8(0, p.brightness);
}

void test_canvas_pixel_at_out_of_bounds_returns_zero() {
    TEST_ASSERT_TRUE(canvas.init(10, 10));
    canvas.setPixel(9, 9, 255, 0, 0, 31);
    Pixel p = canvas.pixelAt(10, 10);
    TEST_ASSERT_EQUAL_UINT8(0, p.brightness);
    TEST_ASSERT_EQUAL_UINT8(0, p.red);
}

void test_canvas_set_pixel_out_of_bounds_ignored() {
    TEST_ASSERT_TRUE(canvas.init(10, 10));
    canvas.setPixel(100, 100, 255, 0, 0, 16);
    Pixel p = canvas.pixelAt(9, 9);
    TEST_ASSERT_EQUAL_UINT8(0, p.brightness);
}

// ---- PolarTransform tests ----

void test_polar_empty_canvas_produces_empty_fb() {
    TEST_ASSERT_TRUE(canvas.init(20, 20));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    PolarTransform pt;
    pt.buildLut(36, 10, 20, 20);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    for (uint16_t s = 0; s < 36; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 10; led++) {
            TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix, slice[led].brightness);
        }
    }
}

void test_polar_center_pixel_maps_to_inner_leds() {
    uint16_t cw = 20, ch = 20;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    canvas.setPixel(cw / 2, ch / 2, 255, 0, 0, 16);

    PolarTransform pt;
    pt.buildLut(36, 10, cw, ch);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    int litAtLed0 = 0;
    for (uint16_t s = 0; s < 36; s++) {
        const Pixel* slice = fb.getSlice(s);
        if (slice[0].brightness != kHd107sBrightnessPrefix) litAtLed0++;
    }
    TEST_ASSERT_GREATER_THAN(0, litAtLed0);
}

void test_polar_right_edge_maps_to_slice0_outer_led() {
    uint16_t cw = 20, ch = 20;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    canvas.setPixel(cw - 1, ch / 2, 255, 0, 0, 16);

    PolarTransform pt;
    pt.buildLut(36, 10, cw, ch);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    bool foundOuterLed = false;
    for (uint16_t s = 0; s < 36; s++) {
        const Pixel* slice = fb.getSlice(s);
        if (slice[9].brightness != kHd107sBrightnessPrefix) {
            foundOuterLed = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(foundOuterLed,
        "Right edge pixel should map to outer LED ring");
}

void test_polar_brightness_preserved() {
    uint16_t cw = 20, ch = 20;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    canvas.setPixel(cw - 1, ch / 2, 200, 100, 50, 25);

    PolarTransform pt;
    pt.buildLut(36, 10, cw, ch);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    bool found = false;
    for (uint16_t s = 0; s < 36 && !found; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 10; led++) {
            if (slice[led].brightness != kHd107sBrightnessPrefix) {
                TEST_ASSERT_EQUAL_UINT8(200, slice[led].red);
                TEST_ASSERT_EQUAL_UINT8(100, slice[led].green);
                TEST_ASSERT_EQUAL_UINT8(50, slice[led].blue);
                TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix | 25, slice[led].brightness);
                found = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found, "Should find at least one lit pixel");
}

void test_polar_full_canvas_fills_all_fb_pixels() {
    uint16_t cw = 20, ch = 20;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    for (uint16_t x = 0; x < cw; x++)
        for (uint16_t y = 0; y < ch; y++)
            canvas.setPixel(x, y, 255, 0, 0, 16);

    PolarTransform pt;
    pt.buildLut(36, 10, cw, ch);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    int litCount = 0;
    for (uint16_t s = 0; s < 36; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 10; led++) {
            if (slice[led].brightness != kHd107sBrightnessPrefix) litCount++;
        }
    }
    TEST_ASSERT_EQUAL(36 * 10, litCount);
}

void test_polar_lut_rebuild_with_new_dimensions() {
    uint16_t cw = 20, ch = 20;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    PolarTransform pt;
    pt.buildLut(36, 10, cw, ch);

    canvas.release();
    fb.release();

    cw = 40; ch = 40;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(72, 20));

    for (uint16_t x = 0; x < cw; x++)
        for (uint16_t y = 0; y < ch; y++)
            canvas.setPixel(x, y, 0, 255, 0, 10);

    pt.buildLut(72, 20, cw, ch);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    int litCount = 0;
    for (uint16_t s = 0; s < 72; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 20; led++) {
            if (slice[led].brightness != kHd107sBrightnessPrefix) litCount++;
        }
    }
    TEST_ASSERT_EQUAL(72 * 20, litCount);
}

void test_polar_angular_symmetry() {
    uint16_t cw = 40, ch = 40;
    TEST_ASSERT_TRUE(canvas.init(cw, ch));
    TEST_ASSERT_TRUE(fb.init(360, 10));

    for (uint16_t x = cw / 2; x < cw; x++)
        for (uint16_t y = 0; y < ch; y++)
            canvas.setPixel(x, y, 255, 0, 0, 16);

    PolarTransform pt;
    pt.buildLut(360, 10, cw, ch);
    fb.clearBack();
    pt.apply(canvas, fb, cfg);
    fb.swap();

    int litRight = 0;
    int litLeft  = 0;
    for (uint16_t s = 0; s < 90; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 10; led++)
            if (slice[led].brightness != kHd107sBrightnessPrefix) litRight++;
    }
    for (uint16_t s = 90; s < 270; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 10; led++)
            if (slice[led].brightness != kHd107sBrightnessPrefix) litLeft++;
    }
    TEST_ASSERT_GREATER_THAN(0, litRight);
    TEST_ASSERT_GREATER_THAN(litLeft, litRight);
}

// ---- IdentityTransform tests ----

void test_identity_copies_pixels_1to1() {
    TEST_ASSERT_TRUE(canvas.init(36, 10));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    canvas.setPixel(5, 3, 100, 200, 50, 20);

    IdentityTransform it;
    fb.clearBack();
    it.apply(canvas, fb, cfg);
    fb.swap();

    const Pixel* slice = fb.getSlice(5);
    TEST_ASSERT_EQUAL_UINT8(100, slice[3].red);
    TEST_ASSERT_EQUAL_UINT8(200, slice[3].green);
    TEST_ASSERT_EQUAL_UINT8(50, slice[3].blue);
    TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix | 20, slice[3].brightness);
}

void test_identity_clamps_to_smaller_dimension() {
    TEST_ASSERT_TRUE(canvas.init(100, 100));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    canvas.setPixel(50, 5, 255, 0, 0, 16);

    IdentityTransform it;
    fb.clearBack();
    it.apply(canvas, fb, cfg);
    fb.swap();

    const Pixel* slice = fb.getSlice(35);
    for (uint16_t led = 0; led < 10; led++) {
        TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix, slice[led].brightness);
    }
}

void test_identity_empty_canvas_produces_empty_fb() {
    TEST_ASSERT_TRUE(canvas.init(36, 10));
    TEST_ASSERT_TRUE(fb.init(36, 10));

    IdentityTransform it;
    fb.clearBack();
    it.apply(canvas, fb, cfg);
    fb.swap();

    for (uint16_t s = 0; s < 36; s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t led = 0; led < 10; led++) {
            TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix, slice[led].brightness);
        }
    }
}

int main() {
    UNITY_BEGIN();

    // Canvas
    RUN_TEST(test_canvas_init_sets_dimensions);
    RUN_TEST(test_canvas_init_clears_buffer);
    RUN_TEST(test_canvas_set_pixel_stores_rgba);
    RUN_TEST(test_canvas_brightness_encoding);
    RUN_TEST(test_canvas_clear_zeros_pixels);
    RUN_TEST(test_canvas_pixel_at_out_of_bounds_returns_zero);
    RUN_TEST(test_canvas_set_pixel_out_of_bounds_ignored);

    // PolarTransform
    RUN_TEST(test_polar_empty_canvas_produces_empty_fb);
    RUN_TEST(test_polar_center_pixel_maps_to_inner_leds);
    RUN_TEST(test_polar_right_edge_maps_to_slice0_outer_led);
    RUN_TEST(test_polar_brightness_preserved);
    RUN_TEST(test_polar_full_canvas_fills_all_fb_pixels);
    RUN_TEST(test_polar_lut_rebuild_with_new_dimensions);
    RUN_TEST(test_polar_angular_symmetry);

    // IdentityTransform
    RUN_TEST(test_identity_copies_pixels_1to1);
    RUN_TEST(test_identity_clamps_to_smaller_dimension);
    RUN_TEST(test_identity_empty_canvas_produces_empty_fb);

    return UNITY_END();
}
