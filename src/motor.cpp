#include "motor.h"
#include <Arduino.h>

static constexpr uint32_t ESC_FREQ       = 50;   // 50 Hz standard servo
static constexpr uint8_t  ESC_RESOLUTION = 16;   // 16-bit timer
// At 50 Hz / 16-bit: 1 tick = 20000µs / 65536 ≈ 0.305µs
static constexpr uint32_t US_TO_DUTY(uint16_t us) {
    return (uint32_t)us * 65536 / 20000;
}

void Motor::init(uint8_t pin) {
    pin_ = pin;
    ledcAttach(pin_, ESC_FREQ, ESC_RESOLUTION);
    stop();
}

void Motor::arm() {
    ledcWrite(pin_, US_TO_DUTY(1000));
    delay(3000);
}

void Motor::setPulseUs(uint16_t pulseUs) {
    if (pulseUs < 1000) pulseUs = 1000;
    if (pulseUs > 2000) pulseUs = 2000;
    ledcWrite(pin_, US_TO_DUTY(pulseUs));
}

void Motor::stop() {
    ledcWrite(pin_, US_TO_DUTY(1000));
}
