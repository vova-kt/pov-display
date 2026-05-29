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

#ifndef HW_NUM_LEDS
#define HW_NUM_LEDS 40 // Physical LEDs mounted on the strip
#endif
#ifndef HW_STRIP_REVERSED
#define HW_STRIP_REVERSED 0 // 1 when strip DI starts at outer tip
#endif
#ifndef HW_SPI_CLOCK_MHZ
#define HW_SPI_CLOCK_MHZ 20 // HD107S SPI clock in MHz
#endif
#ifndef HW_MAX_BRIGHTNESS
#define HW_MAX_BRIGHTNESS 31 // Safety cap for 5-bit HD107S brightness
#endif
#ifndef HW_MAX_RPM
#define HW_MAX_RPM 900 // Safety cap for closed-loop motor speed target
#endif

// --- Hardware geometry (144 LEDs/m strip: 3.0 mm pixel, 3.5 mm gap, 6.5 mm pitch) ---
constexpr float   LED_SIZE_MM    = 3.0f;
constexpr float   LED_GAP_MM     = 3.5f;
constexpr uint8_t HUB_RADIUS_MM  = 1;

// --- ESC pulse bounds and closed-loop motor control ---
static constexpr uint16_t kStopPulseUs              = 1000; // ESC calibrated minimum/stop pulse
static constexpr uint16_t kMotorStartupPulseUs      = 1160; // conservative first pulse, 20us above 1140 min-to-spin
static constexpr uint16_t kMaxPulseUs               = 2000; // ESC calibrated maximum pulse
// Blind-ramp cap: must stay above kMotorStartupPulseUs but below the
// steady-state pulse for the default target RPM.  When changing the default
// targetHz (currently 12 Hz → 360 RPM with 2 arms), re-measure steady-state
// pulse at the new RPM and set this ~40-60 us above it.
static constexpr uint16_t kNoHallStartupMaxPulseUs  = 1200;
static constexpr uint8_t  kNoHallRampStepUs         = 5;    // slow startup ramp per control tick
static constexpr uint16_t kMotorControlIntervalMs   = 100;  // throttle correction cadence
static constexpr uint8_t  kMotorControlTaskDelayMs  = 25;   // task poll interval for start/stop changes
static constexpr uint16_t kMotorControlDeadbandRpm  = 15;   // ignore tiny Hall measurement noise
static constexpr uint8_t  kMotorControlRpmPerUs     = 15;   // RPM error represented by one pulse µs step
static constexpr uint8_t  kMotorControlMaxStepUs    = 100;  // largest pulse correction per tick
static constexpr uint16_t kHallFreshTimeoutMs       = 1500; // Hall RPM is stale after this quiet interval
static constexpr uint8_t  kEmaAlphaShift            = 2;    // EMA smoothing: alpha ≈ 1/(1<<2) = 0.25

// --- Runtime configuration ---
struct Config {
    uint16_t numLeds        = HW_NUM_LEDS;
    uint16_t numSlices      = NUM_SLICES;
    uint8_t  brightness     = 2;     // 0..31 HD107S global brightness
    uint8_t  maxBrightness  = HW_MAX_BRIGHTNESS;
    int16_t  phaseOffset    = 0;

    uint8_t  activePattern  = 3;
    uint8_t  colorR         = 255;
    uint8_t  colorG         = 255;
    uint8_t  colorB         = 255;

    uint8_t  numArms        = NUM_ARMS; // physical arm count (build-time constant)
    uint8_t  targetHz       = 12;     // target refresh rate (12, 24, 25, 30, 60)
    uint16_t escPulseUs     = kStopPulseUs; // current ESC command; motor controller updates while running
    bool     motorStopped   = true;   // transient: default-safe stopped state
    uint8_t  spiClockMhz    = HW_SPI_CLOCK_MHZ;
    bool     mirrorPattern  = true;
    bool     stripReversed  = HW_STRIP_REVERSED != 0;
    bool     radialBalance  = true;
    uint8_t  logLevel       = 3;      // ESP_LOG_INFO; 1=Error 2=Warn 3=Info 4=Debug

    void loadFromNvs();
    void saveToNvs();
};
