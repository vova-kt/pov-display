#include <unity.h>
#include <cstring>
#include "framebuffer.h"
#include "config.h"
#include "patterns/solid.h"
#include "patterns/rainbow.h"
#include "patterns/scanner.h"
#include "patterns/text.h"
#include "fonts/text_font.h"

static Framebuffer fb;
static Config cfg;

static void setText(TextPattern& tp, const char* s) {
    Param* p = tp.findParam("text");
    if (p) strcpy(p->textBuf, s);
}
static void setMode(TextPattern& tp, int v) { tp.findParam("mode")->value = v; }
static void setDelay(TextPattern& tp, int v) { tp.findParam("delayMs")->value = v; }
static void resizeFramebuffer(uint16_t slices, uint16_t leds) {
    TEST_ASSERT_TRUE(fb.resize(slices, leds));
    cfg.numSlices = slices;
    cfg.numLeds = leds;
}

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
}

void tearDown() {
    fb.release();
}

// ---------- Solid ----------

void test_solid_fills_all_pixels() {
    SolidPattern solid;
    solid.generate(fb, cfg, 0);
    fb.swap();

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
    fb.swap();

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(10, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(20, slice[0].green);
    TEST_ASSERT_EQUAL_UINT8(30, slice[0].blue);
    TEST_ASSERT_EQUAL_UINT8(0xE0 | 5, slice[0].brightness);
}

void test_solid_ignores_time() {
    SolidPattern solid;
    solid.generate(fb, cfg, 0);
    fb.swap();
    uint8_t r0 = fb.getSlice(0)[0].red;

    solid.generate(fb, cfg, 99999);
    fb.swap();
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
    fb.swap();

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
    fb.swap();

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
    fb.swap();
    const Pixel* at0 = fb.getSlice(0);
    uint8_t r0 = at0[0].red, g0 = at0[0].green, b0 = at0[0].blue;

    rainbow.generate(fb, cfg, 1000);
    fb.swap();
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
    fb.swap();

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
    fb.swap();

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(0, slice[1].red);
}

void test_scanner_position_advances() {
    ScannerPattern scanner;
    // delay=32, pos = (32 / 32) % 10 = 1
    scanner.generate(fb, cfg, 32);
    fb.swap();

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(0, slice[0].red);
    TEST_ASSERT_EQUAL_UINT8(255, slice[1].red);
}

void test_scanner_wraps_around() {
    ScannerPattern scanner;
    // delay=32, pos = (320 / 32) % 10 = 0
    scanner.generate(fb, cfg, 320);
    fb.swap();

    const Pixel* slice = fb.getSlice(0);
    TEST_ASSERT_EQUAL_UINT8(255, slice[0].red);
}

void test_scanner_only_one_led_lit() {
    ScannerPattern scanner;
    // delay=32, pos = (64 / 32) % 10 = 2
    scanner.generate(fb, cfg, 64);
    fb.swap();

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
    fb.swap();

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
    setText(text, "");
    text.generate(fb, cfg, 0);
    fb.swap();

    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            TEST_ASSERT_EQUAL_UINT8(0, slice[l].brightness);
        }
    }
}

void test_text_renders_pixels() {
    TextPattern text;
    setText(text, "A");
    text.generate(fb, cfg, 0);
    fb.swap();

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
    setText(text, "I");
    text.generate(fb, cfg, 0);
    fb.swap();

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

    TextPattern text;
    setText(text, "A");
    text.generate(fb, cfg, 0);
    fb.swap();

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
    setText(text, "H");
    text.generate(fb, cfg, 0);
    fb.swap();

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

// ---------- Text Spell Mode ----------

static int countLitPixels(Framebuffer& fb) {
    int count = 0;
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        for (uint16_t l = 0; l < fb.numLeds(); l++) {
            if (slice[l].brightness != 0) count++;
        }
    }
    return count;
}

static int countLitOnLed(Framebuffer& fb, uint16_t led) {
    int count = 0;
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        if (fb.getSlice(s)[led].brightness != 0) count++;
    }
    return count;
}

static void capturePixels(Pixel* out) {
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        memcpy(out + s * fb.numLeds(), fb.getSlice(s), fb.numLeds() * sizeof(Pixel));
    }
}

static bool pixelsEqual(const Pixel* a, const Pixel* b) {
    return memcmp(a, b, fb.numSlices() * fb.numLeds() * sizeof(Pixel)) == 0;
}

void test_low_scale_odd_text_dodges_center_gap() {
    resizeFramebuffer(72, 36);

    TextPattern text;
    setText(text, "IIIII");
    text.generate(fb, cfg, 0);
    fb.swap();

    TEST_ASSERT_GREATER_THAN(0, countLitPixels(fb));
    TEST_ASSERT_EQUAL_INT(0, countLitOnLed(fb, 0));
}

void test_text_renders_cyrillic_pixels() {
    TextPattern text;
    setText(text, "Ж");
    text.generate(fb, cfg, 0);
    fb.swap();

    TEST_ASSERT_GREATER_THAN(0, countLitPixels(fb));
}

void test_text_cache_updates_after_text_change() {
    TextPattern text;
    setText(text, "I");
    text.generate(fb, cfg, 0);
    fb.swap();
    int litI = countLitPixels(fb);

    setText(text, "Ж");
    text.generate(fb, cfg, 0);
    fb.swap();
    int litZh = countLitPixels(fb);

    TEST_ASSERT_NOT_EQUAL(litI, litZh);
}

void test_spell_treats_cyrillic_as_one_character() {
    TextPattern text;
    setMode(text, 1);
    setDelay(text, 100);
    setText(text, "ЖA");

    text.generate(fb, cfg, 0);
    fb.swap();
    TEST_ASSERT_GREATER_THAN(0, countLitPixels(fb));

    text.generate(fb, cfg, 100);
    fb.swap();
    TEST_ASSERT_GREATER_THAN(0, countLitPixels(fb));

    text.generate(fb, cfg, 200);
    fb.swap();
    TEST_ASSERT_EQUAL_INT(0, countLitPixels(fb));
}

void test_wide_cyrillic_glyphs_use_declared_advance() {
    TextFontGlyph glyph;

    TEST_ASSERT_TRUE(textFontGlyph(0x0436, glyph)); // ж
    TEST_ASSERT_EQUAL_UINT8(7, glyph.width);

    TEST_ASSERT_TRUE(textFontGlyph(0x0429, glyph)); // Щ
    TEST_ASSERT_EQUAL_UINT8(7, glyph.width);

    TEST_ASSERT_TRUE(textFontGlyph(0x0428, glyph)); // Ш
    TEST_ASSERT_EQUAL_UINT8(7, glyph.width);

    TEST_ASSERT_TRUE(textFontGlyph(0x0426, glyph)); // Ц
    TEST_ASSERT_EQUAL_UINT8(6, glyph.width);

    TEST_ASSERT_TRUE(textFontGlyph(0x0427, glyph)); // Ч
    TEST_ASSERT_EQUAL_UINT8(6, glyph.width);

    TEST_ASSERT_TRUE(textFontGlyph(0x044B, glyph)); // ы
    TEST_ASSERT_EQUAL_UINT8(6, glyph.width);

    TextFontRun run;
    textFontDecodeRun("ЖЫ", strlen("ЖЫ"), run);
    TEST_ASSERT_EQUAL_UINT8(2, run.count);
    TEST_ASSERT_EQUAL_UINT16(14, run.width);
}

void test_spell_produces_different_frames() {
    TextPattern text;
    setMode(text, 1);
    setDelay(text, 100);
    setText(text, "ABC");

    text.generate(fb, cfg, 0);
    fb.swap();
    int lit0 = countLitPixels(fb);
    TEST_ASSERT_GREATER_THAN(0, lit0);

    text.generate(fb, cfg, 100);
    fb.swap();
    int lit100 = countLitPixels(fb);

    TEST_ASSERT_NOT_EQUAL(lit0, lit100);
}

void test_spell_loops_after_full_reveal() {
    TextPattern text;
    setMode(text, 1);
    setDelay(text, 100);
    setText(text, "AB");
    // cycle = len(2) + 2 = 4 steps. At step 4 (400ms) it wraps to step 0 (1 char).
    text.generate(fb, cfg, 400);
    fb.swap();
    int litWrapped = countLitPixels(fb);

    text.generate(fb, cfg, 0);
    fb.swap();
    int litStart = countLitPixels(fb);

    TEST_ASSERT_EQUAL_INT(litStart, litWrapped);
}

// ---------- Text Word Mode ----------

void test_word_shows_single_word_at_each_step() {
    Pixel wordStep0[36 * 10];
    Pixel wordStep1[36 * 10];
    Pixel staticHi[36 * 10];
    Pixel staticWorld[36 * 10];

    TextPattern wordMode;
    setMode(wordMode, 2);
    setDelay(wordMode, 100);
    setText(wordMode, "HI  WORLD");

    wordMode.generate(fb, cfg, 0);
    fb.swap();
    capturePixels(wordStep0);

    wordMode.generate(fb, cfg, 100);
    fb.swap();
    capturePixels(wordStep1);

    TextPattern ref;
    setText(ref, "HI");
    ref.generate(fb, cfg, 0);
    fb.swap();
    capturePixels(staticHi);

    setText(ref, "WORLD");
    ref.generate(fb, cfg, 0);
    fb.swap();
    capturePixels(staticWorld);

    TEST_ASSERT_TRUE(pixelsEqual(wordStep0, staticHi));
    TEST_ASSERT_TRUE(pixelsEqual(wordStep1, staticWorld));
}

void test_word_mode_blanks_between_cycles() {
    TextPattern text;
    setMode(text, 2);
    setDelay(text, 100);
    setText(text, "HI WORLD");

    text.generate(fb, cfg, 200);
    fb.swap();

    TEST_ASSERT_EQUAL_INT(0, countLitPixels(fb));
}

// ---------- Text Marquee Mode ----------

void test_marquee_changes_over_time() {
    TextPattern text;
    setMode(text, 3);
    setDelay(text, 100);
    setText(text, "SCROLL");

    text.generate(fb, cfg, 0);
    fb.swap();

    uint8_t snapshot0[36 * 10 * 4];
    for (uint16_t s = 0; s < fb.numSlices(); s++) {
        const Pixel* slice = fb.getSlice(s);
        memcpy(snapshot0 + s * fb.numLeds() * 4, slice, fb.numLeds() * sizeof(Pixel));
    }

    text.generate(fb, cfg, 2000);
    fb.swap();

    bool different = false;
    for (uint16_t s = 0; s < fb.numSlices() && !different; s++) {
        const Pixel* slice = fb.getSlice(s);
        const Pixel* prev = (const Pixel*)(snapshot0 + s * fb.numLeds() * 4);
        for (uint16_t l = 0; l < fb.numLeds() && !different; l++) {
            if (slice[l].red != prev[l].red || slice[l].green != prev[l].green ||
                slice[l].blue != prev[l].blue || slice[l].brightness != prev[l].brightness)
                different = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(different, "Marquee should produce different frames at different times");
}

void test_marquee_renders_pixels() {
    TextPattern text;
    setMode(text, 3);
    setDelay(text, 100);
    setText(text, "HELLO");

    // Marquee starts text off-screen right; advance enough for text to scroll into view
    text.generate(fb, cfg, 5000);
    fb.swap();
    TEST_ASSERT_GREATER_THAN(0, countLitPixels(fb));
}

// ---------- Text static mode unchanged ----------

void test_static_mode_ignores_time() {
    TextPattern text;
    setMode(text, 0);
    setText(text, "A");

    text.generate(fb, cfg, 0);
    fb.swap();
    int lit0 = countLitPixels(fb);

    text.generate(fb, cfg, 99999);
    fb.swap();
    int litLater = countLitPixels(fb);

    TEST_ASSERT_EQUAL_INT(lit0, litLater);
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
    RUN_TEST(test_low_scale_odd_text_dodges_center_gap);
    RUN_TEST(test_text_renders_cyrillic_pixels);
    RUN_TEST(test_text_cache_updates_after_text_change);
    RUN_TEST(test_spell_treats_cyrillic_as_one_character);
    RUN_TEST(test_wide_cyrillic_glyphs_use_declared_advance);

    RUN_TEST(test_spell_produces_different_frames);
    RUN_TEST(test_spell_loops_after_full_reveal);
    RUN_TEST(test_word_shows_single_word_at_each_step);
    RUN_TEST(test_word_mode_blanks_between_cycles);
    RUN_TEST(test_marquee_changes_over_time);
    RUN_TEST(test_marquee_renders_pixels);
    RUN_TEST(test_static_mode_ignores_time);

    return UNITY_END();
}
