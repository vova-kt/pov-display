#pragma once
#include <cstdint>

class Motor {
public:
    void init(uint8_t pin);
    void arm();
    void setPulseUs(uint16_t pulseUs);
    void stop();

private:
    uint8_t pin_ = 0;
};
