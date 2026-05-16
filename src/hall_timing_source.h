#pragma once
#include "timing_source.h"
#include "hall_sensor.h"

class HallTimingSource : public TimingSource {
public:
    explicit HallTimingSource(HallSensor* hall) : hall_(hall) {}
    bool     consumeNewRotation() override { return hall_->consumeNewRotation(); }
    uint32_t rotationPeriodUs() const override { return hall_->rotationPeriodUs(); }
    uint32_t lastTriggerMs()    const override { return hall_->lastTriggerMs(); }
private:
    HallSensor* hall_;
};
