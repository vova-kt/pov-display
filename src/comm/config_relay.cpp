#include "config_relay.h"
#include "../log_tags.h"
#include "../settings_registry.h"
#include <ArduinoJson.h>
#include <Arduino.h>

LOG_TAG(relay);

// --- Broadcaster (stationary MCU) ---

void ConfigRelayBroadcaster::init(uint16_t port) {
    port_ = port;
    udp_.begin(port_);
}

void ConfigRelayBroadcaster::send(const char* json, size_t len) {
    if (!json || len == 0) return;

    IPAddress broadcastAddr(192, 168, 4, 255);
    udp_.beginPacket(broadcastAddr, port_);
    udp_.write(reinterpret_cast<const uint8_t*>(json), len);
    udp_.endPacket();
}

// --- Receiver (rotating MCU) ---

void ConfigRelayReceiver::init(uint16_t port) {
    udp_.begin(port);
}

void ConfigRelayReceiver::poll() {
    int len = udp_.parsePacket();
    if (len <= 0) return;

    char buf[2048];
    int n = udp_.read(reinterpret_cast<uint8_t*>(buf), sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = '\0';

    JsonDocument doc;
    if (deserializeJson(doc, buf, n) != DeserializationError::Ok) {
        POV_LOGW("bad JSON");
        return;
    }

    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
    POV_LOGI("applied %d bytes", n);

    if (cb_) cb_();
}
