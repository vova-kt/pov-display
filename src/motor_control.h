#pragma once

#include <cstdint>
#include "config.h"

uint32_t targetRefreshHzToRpm(uint8_t targetHz, uint8_t numArms);
uint32_t freshHallRpm(uint32_t measuredRpm, uint32_t lastTriggerMs, uint32_t nowMs);

class MotorSpeedController {
public:
    void start(uint8_t targetHz, uint8_t numArms);
    void stop();
    void setTarget(uint8_t targetHz, uint8_t numArms);
    uint16_t update(uint32_t measuredRpm);

    bool running() const { return running_; }
    uint32_t targetRpm() const { return targetRpm_; }
    uint16_t pulseUs() const { return pulseUs_; }

private:
    bool     running_   = false;
    uint32_t targetRpm_ = 0;
    uint16_t pulseUs_   = kStopPulseUs;
};
