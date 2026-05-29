#pragma once
#include <cstdint>
#include <Arduino.h>

// --- Pin assignments (overridable via -DPIN_xxx=Dy build flags) ---
#ifndef PIN_LED_CLK
constexpr uint8_t PIN_LED_CLK  = D8;   // SPI SCK
#endif
#ifndef PIN_LED_MOSI
constexpr uint8_t PIN_LED_MOSI = D10;  // SPI MOSI
#endif
#ifndef PIN_HALL
constexpr uint8_t PIN_HALL     = D2;
#endif
#ifndef PIN_ESC
constexpr uint8_t PIN_ESC      = D3;
#endif

// --- Compile-time limits ---
#ifndef MAX_LEDS
#define MAX_LEDS 72
#endif
#ifndef MAX_SLICES
#define MAX_SLICES 720
#endif
#ifndef NUM_SLICES
#define NUM_SLICES 360
#endif
#ifndef NUM_ARMS
constexpr uint8_t NUM_ARMS = 2;    // physical arm count (overridable via -DNUM_ARMS=N)
#endif


// --- Hardware geometry (144 LEDs/m strip: 3.0 mm pixel, 3.5 mm gap, 6.5 mm pitch) ---
constexpr float   LED_SIZE_MM    = 3.0f;
constexpr float   LED_GAP_MM     = 3.5f;
constexpr uint8_t HUB_RADIUS_MM  = 1;

// --- ESC pulse mapping (linear: 1150 µs at 0 RPM spin threshold → 2000 µs at 3600 RPM) ---
static constexpr uint16_t kStopPulseUs    = 1000;
static constexpr uint16_t kMinSpinPulseUs = 1150;
static constexpr uint16_t kMaxPulseUs     = 2000;
static constexpr uint32_t kMaxRpm         = 3600; // 60 Hz × 60 s / 1 arm

inline uint16_t rpmToPulseUs(uint32_t rpm) {
    if (rpm == 0) return kStopPulseUs;
    uint16_t pulse = kMinSpinPulseUs + (uint16_t)(rpm * (kMaxPulseUs - kMinSpinPulseUs) / kMaxRpm);
    return pulse > kMaxPulseUs ? kMaxPulseUs : pulse;
}

// --- Runtime configuration ---
struct Config {
    uint16_t numLeds        = 40;
    uint16_t numSlices      = NUM_SLICES;
    uint8_t  brightness     = 16;     // 0..31 HD107S global brightness
    uint8_t  maxBrightness  = 31;
    int16_t  phaseOffset    = 0;

    uint8_t  activePattern  = 3;
    uint8_t  colorR         = 255;
    uint8_t  colorG         = 0;
    uint8_t  colorB         = 0;

    uint8_t  numArms        = NUM_ARMS; // physical arm count (build-time constant)
    uint8_t  targetHz       = 12;     // target refresh rate (12, 24, 25, 30, 60)
    uint16_t escPulseUs     = kStopPulseUs; // derived from targetHz; 1000=stop
    bool     motorStopped   = false;  // transient: true when user pressed Stop
    uint8_t  spiClockMhz    = 20;
    bool     mirrorPattern  = true;
    bool     stripReversed  = false;
    bool     radialBalance  = true;

    void loadFromNvs();
    void saveToNvs();
};
