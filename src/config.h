#pragma once
#include <cstdint>

// --- Pin assignments ---
constexpr uint8_t PIN_LED_CLK  = 18;  // VSPI_CLK
constexpr uint8_t PIN_LED_MOSI = 23;  // VSPI_MOSI
constexpr uint8_t PIN_HALL     = 27;
constexpr uint8_t PIN_ESC      = 25;

// --- Compile-time limits ---
#ifndef MAX_LEDS
#define MAX_LEDS 144
#endif
#ifndef MAX_SLICES
#define MAX_SLICES 720
#endif

// --- Runtime configuration ---
struct Config {
    uint16_t numLeds        = 144;
    uint16_t numSlices      = 360;
    uint8_t  brightness     = 16;     // 0..31 HD107S global brightness
    uint8_t  maxBrightness  = 31;
    int16_t  phaseOffset    = 0;

    uint8_t  activePattern  = 3;
    uint8_t  colorR         = 255;
    uint8_t  colorG         = 0;
    uint8_t  colorB         = 0;
    char     text[64]       = "HELLO";

    uint16_t escPulseUs     = 1000;   // 1000=stop, 2000=full
    uint8_t  spiClockMhz    = 20;

    void loadFromNvs();
    void saveToNvs();
};
