#pragma once
#include <cstdint>
#include <driver/spi_master.h>
#include "config.h"
#include "framebuffer.h"

class LedDriver {
public:
    bool init(uint8_t clkPin, uint8_t mosiPin, uint8_t clockMhz, uint16_t maxLeds);
    void sendSlice(const Pixel* pixels, uint16_t count);
    void allOff(uint16_t count);
    void recomputeScale(uint16_t numLeds, bool radialBalance);
    void setReversed(bool rev) { reversed_ = rev; }

private:
    spi_device_handle_t spi_ = nullptr;
    uint8_t* txBuf_          = nullptr;
    uint16_t maxLeds_        = 0;
    uint8_t  outputScale_[MAX_LEDS];
    bool     applyScale_     = false;
    bool     reversed_       = false;

    uint16_t buildFrame(const Pixel* pixels, uint16_t count);
};
