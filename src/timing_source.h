#pragma once
#include <cstdint>

class TimingSource {
public:
    virtual ~TimingSource() = default;
    virtual bool     consumeNewRotation() = 0;
    virtual uint32_t rotationPeriodUs() const = 0;
    virtual uint32_t lastTriggerMs()    const = 0;
};
