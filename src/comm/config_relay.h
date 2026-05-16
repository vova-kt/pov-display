#pragma once
#include <cstdint>
#include <WiFi.h>
#include <WiFiUdp.h>

constexpr uint16_t CONFIG_UDP_PORT = 4211;

class ConfigRelayBroadcaster {
public:
    void init(uint16_t port = CONFIG_UDP_PORT);
    void send(const char* json, size_t len);

private:
    WiFiUDP  udp_;
    uint16_t port_ = CONFIG_UDP_PORT;
};

class ConfigRelayReceiver {
public:
    using Callback = void(*)();

    void init(uint16_t port = CONFIG_UDP_PORT);
    void poll();
    void onConfigChange(Callback cb) { cb_ = cb; }

private:
    WiFiUDP  udp_;
    Callback cb_ = nullptr;
};
