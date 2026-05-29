#include <unity.h>
#include "framebuffer.h"

static Framebuffer fb;

void setUp() {
    fb = Framebuffer();
    TEST_ASSERT_TRUE(fb.init(10, 5));
}

void tearDown() {
    fb.release();
}

void test_init_sets_dimensions() {
    TEST_ASSERT_EQUAL_UINT16(10, fb.numSlices());
    TEST_ASSERT_EQUAL_UINT16(5, fb.numLeds());
}

void test_init_clears_buffers() {
    const Pixel* slice = fb.getSlice(0);
    for (uint16_t l = 0; l < fb.numLeds(); l++) {
        TEST_ASSERT_EQUAL_UINT8(0, slice[l].red);
        TEST_ASSERT_EQUAL_UINT8(0, slice[l].green);
        TEST_ASSERT_EQUAL_UINT8(0, slice[l].blue);
        TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix, slice[l].brightness);
    }
}

void test_set_pixel_stores_rgba() {
    fb.setPixel(0, 0, 100, 150, 200, 15);
    Pixel* back = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(100, back[0].red);
    TEST_ASSERT_EQUAL_UINT8(150, back[0].green);
    TEST_ASSERT_EQUAL_UINT8(200, back[0].blue);
}

void test_brightness_encoding() {
    fb.setPixel(0, 0, 0, 0, 0, 15);
    Pixel* back = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix | 15, back[0].brightness);
}

void test_brightness_mask() {
    fb.setPixel(0, 0, 0, 0, 0, 0xFF);
    Pixel* back = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0xFF, back[0].brightness);
}

void test_swap_moves_back_to_front() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    fb.swap();
    const Pixel* front = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, front[0].red);
    TEST_ASSERT_EQUAL_UINT8(0, front[0].green);
    TEST_ASSERT_EQUAL_UINT8(0, front[0].blue);
}

void test_double_buffer_isolation() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    const Pixel* front = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0, front[0].red);
}

void test_swap_twice_returns_to_original() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    fb.swap();
    fb.swap();
    Pixel* back = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, back[0].red);
}

void test_clear_back_zeros_pixels() {
    fb.setPixel(0, 0, 255, 128, 64, 31);
    fb.clearBack();
    Pixel* back = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0, back[0].red);
    TEST_ASSERT_EQUAL_UINT8(0, back[0].green);
    TEST_ASSERT_EQUAL_UINT8(0, back[0].blue);
    TEST_ASSERT_EQUAL_UINT8(kHd107sBrightnessPrefix, back[0].brightness);
}

void test_clear_back_preserves_front() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    fb.swap();
    fb.clearBack();
    const Pixel* front = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, front[0].red);
}

void test_get_slice_different_slices() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    fb.setPixel(1, 0, 0, 255, 0, 31);
    fb.swap();

    const Pixel* s0 = fb.getSlice(0);
    const Pixel* s1 = fb.getSlice(1);
    TEST_ASSERT_EQUAL_UINT8(255, s0[0].red);
    TEST_ASSERT_EQUAL_UINT8(0, s0[0].green);
    TEST_ASSERT_EQUAL_UINT8(0, s1[0].red);
    TEST_ASSERT_EQUAL_UINT8(255, s1[0].green);
}

void test_multiple_leds_in_slice() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    fb.setPixel(0, 4, 0, 0, 255, 31);
    fb.swap();

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(255, slice[4].blue);
    TEST_ASSERT_EQUAL_UINT8(0, slice[2].red);
}

void test_resize() {
    TEST_ASSERT_TRUE(fb.resize(20, 10));
    TEST_ASSERT_EQUAL_UINT16(20, fb.numSlices());
    TEST_ASSERT_EQUAL_UINT16(10, fb.numLeds());

    fb.setPixel(19, 9, 100, 200, 50, 16);
    fb.swap();
    const Pixel* slice = fb.getSlice(19);
    TEST_ASSERT_EQUAL_UINT8(100, slice[9].red);
    TEST_ASSERT_EQUAL_UINT8(200, slice[9].green);
    TEST_ASSERT_EQUAL_UINT8(50, slice[9].blue);
}

void test_resize_clears_buffers() {
    fb.setPixel(0, 0, 255, 0, 0, 31);
    fb.swap();

    TEST_ASSERT_TRUE(fb.resize(10, 5));
    const Pixel* front = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0, front[0].red);
}

void test_back_buffer_pointer() {
    Pixel* back = fb.backBuffer();
    TEST_ASSERT_NOT_NULL(back);
    back[0].red = 42;
    Pixel* backSlice = fb.backSlice(0);
    TEST_ASSERT_EQUAL_UINT8(42, backSlice[0].red);
}

void test_pixel_struct_size() {
    TEST_ASSERT_EQUAL(4, sizeof(Pixel));
}

static void assertAllBlack(Framebuffer& f, const char* msg) {
    for (uint16_t s = 0; s < f.numSlices(); s++) {
        const Pixel* slice = f.getSlice(s);
        for (uint16_t l = 0; l < f.numLeds(); l++) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(kHd107sBrightnessPrefix, slice[l].brightness, msg);
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, slice[l].red, msg);
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, slice[l].green, msg);
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, slice[l].blue, msg);
        }
    }
}

void test_clear_single_pixel_buffer() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(1, 1));
    fb.setPixel(0, 0, 255, 128, 64, 31);
    fb.swap();
    fb.clearBack();
    fb.swap();
    assertAllBlack(fb, "1x1");
}

void test_clear_two_pixel_buffer() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(1, 2));
    fb.clearBack();
    fb.swap();
    assertAllBlack(fb, "1x2");
}

void test_clear_three_pixel_buffer() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(1, 3));
    fb.clearBack();
    fb.swap();
    assertAllBlack(fb, "1x3");
}

void test_clear_power_of_two_buffer() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(4, 8));
    fb.clearBack();
    fb.swap();
    assertAllBlack(fb, "4x8");
}

void test_clear_non_power_of_two_buffer() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(7, 13));
    fb.clearBack();
    fb.swap();
    assertAllBlack(fb, "7x13");
}

void test_init_fills_both_buffers() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(5, 3));
    assertAllBlack(fb, "front after init");
    fb.swap();
    assertAllBlack(fb, "back after init");
}

void test_resize_fills_non_power_of_two() {
    TEST_ASSERT_TRUE(fb.resize(11, 7));
    assertAllBlack(fb, "front after resize 11x7");
    fb.swap();
    assertAllBlack(fb, "back after resize 11x7");
}

void test_clear_overwrites_dirty_buffer() {
    fb.release();
    TEST_ASSERT_TRUE(fb.init(6, 5));
    for (uint16_t s = 0; s < 6; s++)
        for (uint16_t l = 0; l < 5; l++)
            fb.setPixel(s, l, 0xFF, 0xFF, 0xFF, 31);
    fb.clearBack();
    fb.swap();
    assertAllBlack(fb, "clear after full dirty");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_dimensions);
    RUN_TEST(test_init_clears_buffers);
    RUN_TEST(test_set_pixel_stores_rgba);
    RUN_TEST(test_brightness_encoding);
    RUN_TEST(test_brightness_mask);
    RUN_TEST(test_swap_moves_back_to_front);
    RUN_TEST(test_double_buffer_isolation);
    RUN_TEST(test_swap_twice_returns_to_original);
    RUN_TEST(test_clear_back_zeros_pixels);
    RUN_TEST(test_clear_back_preserves_front);
    RUN_TEST(test_get_slice_different_slices);
    RUN_TEST(test_multiple_leds_in_slice);
    RUN_TEST(test_resize);
    RUN_TEST(test_resize_clears_buffers);
    RUN_TEST(test_back_buffer_pointer);
    RUN_TEST(test_pixel_struct_size);
    RUN_TEST(test_clear_single_pixel_buffer);
    RUN_TEST(test_clear_two_pixel_buffer);
    RUN_TEST(test_clear_three_pixel_buffer);
    RUN_TEST(test_clear_power_of_two_buffer);
    RUN_TEST(test_clear_non_power_of_two_buffer);
    RUN_TEST(test_init_fills_both_buffers);
    RUN_TEST(test_resize_fills_non_power_of_two);
    RUN_TEST(test_clear_overwrites_dirty_buffer);
    return UNITY_END();
}
