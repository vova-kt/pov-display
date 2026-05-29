#include "motor.h"
#include "log_tags.h"
#include <Arduino.h>

LOG_TAG(motor);

static constexpr uint32_t ESC_FREQ       = 50;   // 50 Hz standard servo
static constexpr uint8_t  ESC_RESOLUTION = 16;   // 16-bit timer
// At 50 Hz / 16-bit: 1 tick = 20000µs / 65536 ≈ 0.305µs
static constexpr uint32_t US_TO_DUTY(uint16_t us) {
    return (uint32_t)us * 65536 / 20000;
}

void Motor::init(uint8_t pin) {
    pin_ = pin;
    POV_LOGI("ledcAttach pin=%u freq=%uHz res=%ubit", pin_, ESC_FREQ, ESC_RESOLUTION);
    bool ok = ledcAttach(pin_, ESC_FREQ, ESC_RESOLUTION);
    POV_LOGI("ledcAttach: %s", ok ? "OK" : "FAILED");
}

void Motor::arm() {
    POV_LOGI("sending 2000us (max) for calibration...");
    ledcWrite(pin_, US_TO_DUTY(2000));
    delay(500);

    POV_LOGI("sending 1000us (min) to complete calibration + arm...");
    ledcWrite(pin_, US_TO_DUTY(1000));
    delay(4000);
    POV_LOGI("arm done");
}

void Motor::calibrateRange() {
    arm();
}

void Motor::setPulseUs(uint16_t pulseUs) {
    uint16_t orig = pulseUs;
    if (pulseUs < 1000) pulseUs = 1000;
    if (pulseUs > 2000) pulseUs = 2000;
    uint32_t duty = US_TO_DUTY(pulseUs);
    float pct = (pulseUs - 1000) / 10.0f;
    POV_LOGD("setPulse: requested=%uus clamped=%uus duty=%lu/65536 throttle=%.1f%%",
             orig, pulseUs, duty, pct);
    ledcWrite(pin_, duty);
}

void Motor::stop() {
    uint32_t duty = US_TO_DUTY(1000);
    POV_LOGD("stop: 1000us duty=%lu/65536", duty);
    ledcWrite(pin_, duty);
}
