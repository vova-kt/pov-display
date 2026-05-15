#pragma once
#include "font5x7_ascii.h"
#include "font5x7_cyrillic.h"
#include <cstdint>
#include <pgmspace.h>

static constexpr uint8_t FONT_CHAR_WIDTH = 5;
static constexpr uint8_t FONT_CHAR_HEIGHT = 7;

static inline bool font5x7GlyphColumns(uint32_t codepoint, uint8_t cols[FONT_CHAR_WIDTH]) {
    if (codepoint >= FONT_5X7_ASCII_FIRST && codepoint <= FONT_5X7_ASCII_LAST) {
        uint16_t index = (uint16_t)(codepoint - FONT_5X7_ASCII_FIRST);
        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++)
            cols[col] = pgm_read_byte(&FONT_5X7_ASCII[index][col]);
        return true;
    }

    if (codepoint >= FONT_5X7_CYRILLIC_FIRST && codepoint <= FONT_5X7_CYRILLIC_LAST) {
        uint16_t index = (uint16_t)(codepoint - FONT_5X7_CYRILLIC_FIRST);
        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++)
            cols[col] = pgm_read_byte(&FONT_5X7_CYRILLIC[index][col]);
        return true;
    }

    if (codepoint == FONT_5X7_CYRILLIC_UPPER_YO ||
        codepoint == FONT_5X7_CYRILLIC_LOWER_YO) {
        for (uint8_t col = 0; col < FONT_CHAR_WIDTH; col++)
            cols[col] = pgm_read_byte(&FONT_5X7_CYRILLIC_YO[col]);
        return true;
    }

    return false;
}
