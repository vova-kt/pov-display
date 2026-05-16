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

    using ConfigCallback = void(*)(void);
    void onConfigChange(ConfigCallback cb) { configCb_ = cb; }

    using RelayCallback = void(*)(const char* json, size_t len);
    void onConfigRelay(RelayCallback cb) { relayCb_ = cb; }

    using ImageCallback = void(*)(const uint8_t* rgbData, uint16_t width, uint16_t height);
    void onImageUpload(ImageCallback cb) { imageCb_ = cb; }

private:
    void setupRoutes();

    AsyncWebServer server_{80};
    Config*        cfg_   = nullptr;
    HallSensor*    hall_  = nullptr;
    Framebuffer*   fb_    = nullptr;
    Motor*         motor_ = nullptr;
    ConfigCallback configCb_ = nullptr;
    RelayCallback  relayCb_  = nullptr;
    ImageCallback  imageCb_  = nullptr;
    SemaphoreHandle_t cfgMutex_ = nullptr;
    String bodyBuffer_;
    bool   bodyOverflow_ = false;
    static constexpr size_t kMaxBodySize = 2048;

    uint8_t* imgBuffer_    = nullptr;
    size_t   imgExpected_  = 0;
    size_t   imgReceived_  = 0;
    uint16_t imgWidth_     = 0;
    uint16_t imgHeight_    = 0;
    static constexpr size_t kMaxImageSize = 288 * 288 * 3;
};
