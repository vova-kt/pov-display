#pragma once
#include "text_font_ascii.h"
#include "text_font_cyrillic.h"
#include <cstdint>
#include <pgmspace.h>

static constexpr uint8_t TEXT_FONT_BASE_WIDTH = 5;
static constexpr uint8_t TEXT_FONT_MAX_GLYPH_WIDTH = 7;
static constexpr uint8_t TEXT_FONT_HEIGHT = 7;
static constexpr uint8_t TEXT_FONT_GAP = 1;
static constexpr uint8_t TEXT_FONT_MAX_RUN_GLYPHS = 64;

struct TextFontMetrics {
    uint8_t height;
    uint8_t maxGlyphWidth;
    uint8_t gap;
};

static constexpr TextFontMetrics TEXT_FONT_METRICS = {
    TEXT_FONT_HEIGHT,
    TEXT_FONT_MAX_GLYPH_WIDTH,
    TEXT_FONT_GAP
};

struct TextFontGlyph {
    uint8_t width;
    uint8_t cols[TEXT_FONT_MAX_GLYPH_WIDTH];
};

struct TextFontRunGlyph {
    TextFontGlyph glyph;
    bool isSpace;
};

struct TextFontRunView {
    const TextFontRunGlyph* glyphs;
    uint8_t count;
    uint16_t width;
};

struct TextFontRun {
    TextFontRunGlyph glyphs[TEXT_FONT_MAX_RUN_GLYPHS];
    uint8_t count;
    uint16_t width;
};

static_assert(sizeof(TextFontRun) <= 640, "TextFontRun must stay small enough for ESP32-C6 SRAM");

static inline void textFontClearGlyph(TextFontGlyph& glyph) {
    glyph.width = TEXT_FONT_BASE_WIDTH;
    for (uint8_t col = 0; col < TEXT_FONT_MAX_GLYPH_WIDTH; col++)
        glyph.cols[col] = 0;
}

static inline bool textFontGlyph(uint32_t codepoint, TextFontGlyph& glyph) {
    textFontClearGlyph(glyph);

    uint8_t width = 0;
    if (textFontCyrillicWideGlyph(codepoint, glyph.cols, width)) {
        glyph.width = width;
        return true;
    }

    if (codepoint >= TEXT_FONT_ASCII_FIRST && codepoint <= TEXT_FONT_ASCII_LAST) {
        uint16_t index = (uint16_t)(codepoint - TEXT_FONT_ASCII_FIRST);
        for (uint8_t col = 0; col < TEXT_FONT_BASE_WIDTH; col++)
            glyph.cols[col] = pgm_read_byte(&TEXT_FONT_ASCII[index][col]);
        return true;
    }

    if (codepoint >= TEXT_FONT_CYRILLIC_FIRST && codepoint <= TEXT_FONT_CYRILLIC_LAST) {
        uint16_t index = (uint16_t)(codepoint - TEXT_FONT_CYRILLIC_FIRST);
        for (uint8_t col = 0; col < TEXT_FONT_BASE_WIDTH; col++)
            glyph.cols[col] = pgm_read_byte(&TEXT_FONT_CYRILLIC[index][col]);
        return true;
    }

    if (codepoint == TEXT_FONT_CYRILLIC_UPPER_YO ||
        codepoint == TEXT_FONT_CYRILLIC_LOWER_YO) {
        for (uint8_t col = 0; col < TEXT_FONT_BASE_WIDTH; col++)
            glyph.cols[col] = pgm_read_byte(&TEXT_FONT_CYRILLIC_YO[col]);
        return true;
    }

    return false;
}

static inline uint32_t textFontNextCodepoint(const char* text, uint16_t len, uint16_t& offset) {
    uint8_t b0 = (uint8_t)text[offset++];
    if (b0 < 0x80) return b0;

    uint8_t extra = 0;
    uint32_t codepoint = 0;
    uint32_t minCodepoint = 0;

    if ((b0 & 0xE0) == 0xC0) {
        extra = 1;
        codepoint = b0 & 0x1F;
        minCodepoint = 0x80;
    } else if ((b0 & 0xF0) == 0xE0) {
        extra = 2;
        codepoint = b0 & 0x0F;
        minCodepoint = 0x800;
    } else if ((b0 & 0xF8) == 0xF0) {
        extra = 3;
        codepoint = b0 & 0x07;
        minCodepoint = 0x10000;
    } else {
        return '?';
    }

    if (offset + extra > len) return '?';

    uint16_t pos = offset;
    for (uint8_t i = 0; i < extra; i++) {
        uint8_t b = (uint8_t)text[pos + i];
        if ((b & 0xC0) != 0x80) return '?';
        codepoint = (codepoint << 6) | (b & 0x3F);
    }
    offset = pos + extra;

    if (codepoint < minCodepoint) return '?';
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return '?';
    if (codepoint > 0x10FFFF) return '?';
    return codepoint;
}

static inline uint16_t textFontMeasureGlyphs(const TextFontRunGlyph* glyphs, uint8_t count) {
    uint16_t width = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (i > 0) width += TEXT_FONT_METRICS.gap;
        width += glyphs[i].glyph.width;
    }
    return width;
}

static inline TextFontRunView textFontRunView(const TextFontRunGlyph* glyphs, uint8_t count) {
    return {glyphs, count, textFontMeasureGlyphs(glyphs, count)};
}

static inline TextFontRunView textFontRunView(const TextFontRun& run) {
    return {run.glyphs, run.count, run.width};
}

static inline void textFontDecodeRun(const char* text, uint16_t len, TextFontRun& run) {
    run.count = 0;
    run.width = 0;

    uint16_t offset = 0;
    while (offset < len && run.count < TEXT_FONT_MAX_RUN_GLYPHS) {
        uint32_t codepoint = textFontNextCodepoint(text, len, offset);

        TextFontRunGlyph& out = run.glyphs[run.count];
        if (!textFontGlyph(codepoint, out.glyph))
            textFontGlyph('?', out.glyph);
        out.isSpace = codepoint == ' ';

        if (run.count > 0) run.width += TEXT_FONT_METRICS.gap;
        run.width += out.glyph.width;
        run.count++;
    }
}
