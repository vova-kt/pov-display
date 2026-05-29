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
    Serial.printf("[motor] ledcAttach pin=%u freq=%uHz res=%ubit\n", pin_, ESC_FREQ, ESC_RESOLUTION);
    bool ok = ledcAttach(pin_, ESC_FREQ, ESC_RESOLUTION);
    Serial.printf("[motor] ledcAttach: %s\n", ok ? "OK" : "FAILED");
    // duty starts at 0 (no pulse) — ESC sees "no signal"
}

void Motor::arm() {
    // Throttle range calibration: max then min. Needed every boot — this ESC
    // doesn't persist calibration. Keep the max phase short to limit spin-up.
    Serial.println("[motor] sending 2000us (max) for calibration...");
    ledcWrite(pin_, US_TO_DUTY(2000));
    delay(500);

    Serial.println("[motor] sending 1000us (min) to complete calibration + arm...");
    ledcWrite(pin_, US_TO_DUTY(1000));
    delay(4000);
    Serial.println("[motor] arm done");
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
    Serial.printf("[motor] setPulse: requested=%uus clamped=%uus duty=%lu/65536 throttle=%.1f%%\n",
                  orig, pulseUs, duty, pct);
    ledcWrite(pin_, duty);
}

void Motor::stop() {
    uint32_t duty = US_TO_DUTY(1000);
    Serial.printf("[motor] stop: 1000us duty=%lu/65536\n", duty);
    ledcWrite(pin_, duty);
}
