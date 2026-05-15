#pragma once
#include "font5x7_ascii.h"
#include "font5x7_cyrillic.h"
#include <cstdint>
#include <pgmspace.h>

static constexpr uint8_t FONT_CHAR_WIDTH = 5;
static constexpr uint8_t FONT_GLYPH_MAX_WIDTH = FONT_5X7_CYRILLIC_WIDE_MAX_WIDTH;
static constexpr uint8_t FONT_CHAR_HEIGHT = 7;
static constexpr uint8_t FONT_GLYPH_GAP = 1;

struct Font5x7Glyph {
    uint8_t width;
    uint8_t cols[FONT_GLYPH_MAX_WIDTH];
};

static inline void font5x7ClearGlyph(Font5x7Glyph& glyph) {
    glyph.width = FONT_CHAR_WIDTH;
    for (uint8_t col = 0; col < FONT_GLYPH_MAX_WIDTH; col++)
        glyph.cols[col] = 0;
}

static inline bool font5x7Glyph(uint32_t codepoint, Font5x7Glyph& glyph) {
    font5x7ClearGlyph(glyph);

    uint8_t width = 0;
    if (font5x7CyrillicWideGlyph(codepoint, glyph.cols, width)) {
        glyph.width = width;
        return true;
    }

    if (codepoint >= FONT_5X7_ASCII_FIRST && codepoint <= FONT_5X7_ASCII_LAST) {
        uint16_t index = (uint16_t)(codepoint - FONT_5X7_ASCII_FIRST);
        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++)
            glyph.cols[col] = pgm_read_byte(&FONT_5X7_ASCII[index][col]);
        return true;
    }

    if (codepoint >= FONT_5X7_CYRILLIC_FIRST && codepoint <= FONT_5X7_CYRILLIC_LAST) {
        uint16_t index = (uint16_t)(codepoint - FONT_5X7_CYRILLIC_FIRST);
        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++)
            glyph.cols[col] = pgm_read_byte(&FONT_5X7_CYRILLIC[index][col]);
        return true;
    }

    if (codepoint == FONT_5X7_CYRILLIC_UPPER_YO ||
        codepoint == FONT_5X7_CYRILLIC_LOWER_YO) {
        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++)
            glyph.cols[col] = pgm_read_byte(&FONT_5X7_CYRILLIC_YO[col]);
        return true;
    }

    return false;
}

static inline uint32_t font5x7NextCodepoint(const char* text, uint16_t len, uint16_t& offset) {
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

static inline bool font5x7NextGlyph(const char* text, uint16_t len,
                                    uint16_t& offset, Font5x7Glyph& glyph) {
    if (offset >= len) return false;
    uint32_t codepoint = font5x7NextCodepoint(text, len, offset);
    if (!font5x7Glyph(codepoint, glyph))
        font5x7Glyph('?', glyph);
    return true;
}

static inline uint16_t font5x7GlyphCount(const char* text, uint16_t len) {
    uint16_t count = 0;
    uint16_t offset = 0;
    while (offset < len) {
        font5x7NextCodepoint(text, len, offset);
        count++;
    }
    return count;
}

static inline uint16_t font5x7Measure(const char* text, uint16_t len) {
    uint16_t cols = 0;
    uint16_t offset = 0;
    Font5x7Glyph glyph;
    while (font5x7NextGlyph(text, len, offset, glyph))
        cols += glyph.width + FONT_GLYPH_GAP;
    return cols > 0 ? cols - FONT_GLYPH_GAP : 0;
}

static inline bool font5x7GlyphByteRange(const char* text, uint16_t len, uint16_t glyphIndex,
                                         uint16_t& byteOffset, uint16_t& byteLen) {
    uint16_t offset = 0;
    for (uint16_t i = 0; offset < len; i++) {
        uint16_t start = offset;
        font5x7NextCodepoint(text, len, offset);
        if (i == glyphIndex) {
            byteOffset = start;
            byteLen = offset - start;
            return true;
        }
    }
    return false;
}
