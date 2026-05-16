#include "wifi_timing.h"
#include <Arduino.h>

// --- Broadcaster (stationary MCU) ---

void WifiTimingBroadcaster::init(uint16_t port) {
    port_ = port;
    udp_.begin(port_);
}

void WifiTimingBroadcaster::broadcast(uint32_t periodUs) {
    TimingPacket pkt;
    pkt.rotPeriodUs = periodUs;
    pkt.senderMs    = millis();

    IPAddress broadcastAddr(192, 168, 4, 255);
    udp_.beginPacket(broadcastAddr, port_);
    udp_.write(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    udp_.endPacket();
}

// --- Receiver (rotating MCU) ---

bool WifiTimingSource::init(uint16_t port) {
    return udp_.begin(port) != 0;
}

void WifiTimingSource::poll() {
    int len = udp_.parsePacket();
    if (len < (int)sizeof(TimingPacket)) return;

    TimingPacket pkt;
    udp_.read(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));

    if (pkt.rotPeriodUs > 0) {
        rotPeriodUs_   = pkt.rotPeriodUs;
        lastTriggerMs_ = millis();
        newRotation_   = true;
    }
}

bool WifiTimingSource::consumeNewRotation() {
    if (!newRotation_) return false;
    newRotation_ = false;
    return true;
}
