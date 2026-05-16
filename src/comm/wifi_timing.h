#pragma once
#include <cstdint>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "../timing_source.h"

constexpr uint16_t TIMING_UDP_PORT = 4210;

struct TimingPacket {
    uint32_t rotPeriodUs;
    uint32_t senderMs;
};

class WifiTimingBroadcaster {
public:
    void init(uint16_t port = TIMING_UDP_PORT);
    void broadcast(uint32_t periodUs);

private:
    WiFiUDP  udp_;
    uint16_t port_ = TIMING_UDP_PORT;
};

class WifiTimingSource : public TimingSource {
public:
    bool init(uint16_t port = TIMING_UDP_PORT);
    void poll();

    bool     consumeNewRotation() override;
    uint32_t rotationPeriodUs() const override { return rotPeriodUs_; }
    uint32_t lastTriggerMs()    const override { return lastTriggerMs_; }

private:
    WiFiUDP           udp_;
    volatile uint32_t rotPeriodUs_   = 0;
    volatile uint32_t lastTriggerMs_ = 0;
    volatile bool     newRotation_   = false;
};
