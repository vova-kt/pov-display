#pragma once
#include <cstdint>
#include <Arduino.h>

// --- Pin assignments (XIAO ESP32-C6 board labels) ---
constexpr uint8_t PIN_LED_CLK  = D8;   // SPI SCK
constexpr uint8_t PIN_LED_MOSI = D10;  // SPI MOSI
constexpr uint8_t PIN_HALL     = D2;
constexpr uint8_t PIN_ESC      = D3;

// --- Compile-time limits ---
#ifndef MAX_LEDS
#define MAX_LEDS 144
#endif
#ifndef MAX_SLICES
#define MAX_SLICES 720
#endif

// --- Runtime configuration ---
struct Config {
    uint16_t numLeds        = 26;
    uint16_t numSlices      = 360;
    uint8_t  brightness     = 16;     // 0..31 HD107S global brightness
    uint8_t  maxBrightness  = 31;
    int16_t  phaseOffset    = -90;

    uint8_t  activePattern  = 1;
    uint8_t  colorR         = 255;
    uint8_t  colorG         = 0;
    uint8_t  colorB         = 0;
    char     text[64]       = "FUSION";

    uint8_t  numArms        = 2;      // 1, 2, or 4 — physical arm count
    uint8_t  targetHz       = 60;     // target refresh rate (12, 24, 25, 30, 60)
    uint16_t escPulseUs     = 1000;   // 1000=stop, 2000=full
    uint8_t  spiClockMhz    = 20;
    bool     mirrorPattern  = true;
    bool     radialBalance  = true;

    void loadFromNvs();
    void saveToNvs();
};
