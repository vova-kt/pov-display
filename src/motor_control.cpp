#include "motor_control.h"

static constexpr uint8_t kSecondsPerMinute = 60; // Hz-to-RPM conversion factor
static constexpr uint8_t kFallbackArmCount = 1;  // protects against invalid zero-arm config

static uint16_t clampPulse(int32_t pulse) {
    if (pulse < kStopPulseUs) return kStopPulseUs;
    if (pulse > kMaxPulseUs) return kMaxPulseUs;
    return (uint16_t)pulse;
}

static int32_t abs32(int32_t v) {
    return v < 0 ? -v : v;
}

uint32_t targetRefreshHzToRpm(uint8_t targetHz, uint8_t numArms) {
    uint8_t arms = numArms <= 0 ? kFallbackArmCount : numArms;
    uint32_t rpm = (uint32_t)targetHz * kSecondsPerMinute / arms;
    return rpm > HW_MAX_RPM ? HW_MAX_RPM : rpm;
}

uint32_t freshHallRpm(uint32_t measuredRpm, uint32_t lastTriggerMs, uint32_t nowMs) {
    if (lastTriggerMs == 0) return 0;
    if (nowMs - lastTriggerMs > kHallFreshTimeoutMs) return 0;
    return measuredRpm;
}

void MotorSpeedController::start(uint8_t targetHz, uint8_t numArms) {
    setTarget(targetHz, numArms);
    if (targetRpm_ == 0) {
        stop();
        return;
    }
    running_ = true;
    pulseUs_ = kMotorStartupPulseUs;
}

void MotorSpeedController::stop() {
    running_ = false;
    targetRpm_ = 0;
    pulseUs_ = kStopPulseUs;
}

void MotorSpeedController::setTarget(uint8_t targetHz, uint8_t numArms) {
    targetRpm_ = targetRefreshHzToRpm(targetHz, numArms);
    if (targetRpm_ == 0) stop();
}

uint16_t MotorSpeedController::update(uint32_t measuredRpm) {
    if (!running_ || targetRpm_ == 0) {
        stop();
        return pulseUs_;
    }

    if (measuredRpm == 0) {
        int32_t next = (int32_t)pulseUs_ + kNoHallRampStepUs;
        pulseUs_ = (uint16_t)(next > kNoHallStartupMaxPulseUs ? kNoHallStartupMaxPulseUs : next);
        return pulseUs_;
    }

    int32_t error = (int32_t)targetRpm_ - (int32_t)measuredRpm;
    if (abs32(error) <= kMotorControlDeadbandRpm) return pulseUs_;

    int32_t step = error / kMotorControlRpmPerUs;
    if (step == 0) step = error > 0 ? 1 : -1;
    if (step > kMotorControlMaxStepUs) step = kMotorControlMaxStepUs;
    if (step < -(int32_t)kMotorControlMaxStepUs) step = -(int32_t)kMotorControlMaxStepUs;

    pulseUs_ = clampPulse((int32_t)pulseUs_ + step);
    return pulseUs_;
}
