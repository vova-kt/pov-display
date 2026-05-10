#pragma once
#include <cstdint>
#include <Arduino.h>

class HallSensor {
public:
    void init(uint8_t pin);

    uint32_t rotationPeriodUs() const { return rotPeriodUs_; }
    uint32_t rpm()              const { return rpm_; }
    uint32_t lastTriggerMs()    const { return lastTriggerMs_; }
    bool     consumeNewRotation();

private:
    static void IRAM_ATTR isr(void* arg);

    volatile uint64_t lastTriggerUs_ = 0;
    volatile uint32_t lastTriggerMs_ = 0;
    volatile uint32_t rotPeriodUs_   = 0;
    volatile uint32_t rpm_           = 0;
    volatile bool     newRotation_   = false;

    static constexpr uint32_t DEBOUNCE_US = 10000;
};
