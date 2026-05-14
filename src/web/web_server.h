#pragma once
#include <ESPAsyncWebServer.h>
#include "../config.h"
#include "../hall_sensor.h"
#include "../framebuffer.h"
#include "../motor.h"

class PovWebServer {
public:
    void init(Config* cfg, HallSensor* hall, Framebuffer* fb, Motor* motor);
    void notifyConfigChanged();

    // Set callback invoked after config is updated via the API
    using ConfigCallback = void(*)(void);
    void onConfigChange(ConfigCallback cb) { configCb_ = cb; }

private:
    void setupRoutes();

    AsyncWebServer server_{80};
    Config*        cfg_   = nullptr;
    HallSensor*    hall_  = nullptr;
    Framebuffer*   fb_    = nullptr;
    Motor*         motor_ = nullptr;
    ConfigCallback configCb_ = nullptr;
    SemaphoreHandle_t cfgMutex_ = nullptr;
    String bodyBuffer_;
    bool   bodyOverflow_ = false;
    static constexpr size_t kMaxBodySize = 2048;
};
