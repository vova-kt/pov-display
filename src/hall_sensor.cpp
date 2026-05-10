#include "hall_sensor.h"
#include <Arduino.h>
#include <esp_timer.h>

void HallSensor::init(uint8_t pin) {
    pinMode(pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(pin), isr, this, FALLING);
}

void IRAM_ATTR HallSensor::isr(void* arg) {
    auto* self = static_cast<HallSensor*>(arg);
    uint64_t now = esp_timer_get_time();
    uint64_t delta = now - self->lastTriggerUs_;

    if (delta < DEBOUNCE_US) return;

    self->rotPeriodUs_   = (uint32_t)delta;
    self->rpm_           = 60000000UL / (uint32_t)delta;
    self->lastTriggerUs_ = now;
    self->lastTriggerMs_ = (uint32_t)(now / 1000);
    self->newRotation_   = true;
}

bool HallSensor::consumeNewRotation() {
    if (!newRotation_) return false;
    newRotation_ = false;
    return true;
}
