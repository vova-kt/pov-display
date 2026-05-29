#include "web_server.h"
#include "web_ui.h"
#include "image_processor_js.h"
#include "settings_js.h"
#include "../effect.h"
#include "../log_tags.h"
#include "../settings_registry.h"
#include "../motor_control.h"
#include "../patterns/registry.h"
#include <ArduinoJson.h>

LOG_TAG(web);

static constexpr uint32_t kRebootDelayMs = 250; // allow HTTP response to flush
static constexpr uint16_t kRebootTaskStackBytes = 2048; // enough for a delay then restart
static constexpr UBaseType_t kRebootTaskPriority = 1; // low-priority one-shot task

static void delayedRebootTask(void*) {
    vTaskDelay(pdMS_TO_TICKS(kRebootDelayMs));
    ESP.restart();
}

void PovWebServer::init(Config* cfg, HallSensor* hall, Framebuffer* fb, Motor* motor) {
    cfg_   = cfg;
    hall_  = hall;
    fb_    = fb;
    motor_ = motor;
    cfgMutex_ = xSemaphoreCreateMutex();
    POV_LOGI("mutex=%s", cfgMutex_ ? "OK" : "FAILED");
    setupRoutes();
    POV_LOGI("routes registered");
    server_.begin();
    POV_LOGI("listening on port 80 (free heap: %u)", ESP.getFreeHeap());
}

void PovWebServer::setupRoutes() {
    server_.onNotFound([](AsyncWebServerRequest* req) {
        ESP_LOGD("web", "404 %s %s", req->methodToString(), req->url().c_str());
        req->send(404, "text/plain", "Not found");
    });

    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        ESP_LOGD("web", "GET / from %s", req->client()->remoteIP().toString().c_str());
        req->send(200, "text/html", INDEX_HTML);
    });

    server_.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        xSemaphoreTake(cfgMutex_, portMAX_DELAY);
        JsonObject root = doc.to<JsonObject>();
        settings_registry::toJson(root, Scope::McuOnly);
        xSemaphoreGive(cfgMutex_);

        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    server_.on("/api/config", HTTP_POST,
        [this](AsyncWebServerRequest* req) {
            if (bodyOverflow_) {
                req->send(413, "application/json", "{\"error\":\"body too large\"}");
                bodyBuffer_ = "";
                bodyOverflow_ = false;
                return;
            }
            ESP_LOGD("web", "POST /api/config (%u bytes)", bodyBuffer_.length());
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyBuffer_);
            if (err) {
                req->send(400, "application/json", "{\"error\":\"bad json\"}");
                bodyBuffer_ = "";
                return;
            }
            xSemaphoreTake(cfgMutex_, portMAX_DELAY);
            settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::McuOnly);
            xSemaphoreGive(cfgMutex_);
            if (relayCb_) relayCb_(bodyBuffer_.c_str(), bodyBuffer_.length());
            bodyBuffer_ = "";

            if (configCb_) configCb_();

            req->send(200, "application/json", "{\"ok\":true}");
        },
        nullptr,
        [this](AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                bodyBuffer_ = "";
                bodyOverflow_ = false;
            }
            if (total > kMaxBodySize) { bodyOverflow_ = true; return; }
            bodyBuffer_ += String((char*)data, len);
        }
    );

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["rpm"]          = freshHallRpm(hall_->rpm(), hall_->lastTriggerMs(), millis());
        doc["freeHeap"]     = ESP.getFreeHeap();
        doc["uptime"]       = millis();
        doc["motorStopped"] = cfg_->motorStopped;
        doc["numArms"]      = cfg_->numArms;
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    server_.on("/api/save", HTTP_POST, [this](AsyncWebServerRequest* req) {
        xSemaphoreTake(cfgMutex_, portMAX_DELAY);
        settings_registry::saveToNvs();
        savePatternsToNvs();
        saveEffectsToNvs();
        xSemaphoreGive(cfgMutex_);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/motor/stop", HTTP_POST, [this](AsyncWebServerRequest* req) {
        ESP_LOGI("web", "POST /api/motor/stop");
        xSemaphoreTake(cfgMutex_, portMAX_DELAY);
        cfg_->motorStopped = true;
        cfg_->escPulseUs = kStopPulseUs;
        xSemaphoreGive(cfgMutex_);
        motor_->stop();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/motor/start", HTTP_POST, [this](AsyncWebServerRequest* req) {
        ESP_LOGI("web", "POST /api/motor/start");
        xSemaphoreTake(cfgMutex_, portMAX_DELAY);
        cfg_->motorStopped = false;
        cfg_->escPulseUs = kMotorStartupPulseUs;
        xSemaphoreGive(cfgMutex_);
        motor_->setPulseUs(cfg_->escPulseUs);
        if (configCb_) configCb_();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        ESP_LOGI("web", "POST /api/reset — restoring defaults");
        xSemaphoreTake(cfgMutex_, portMAX_DELAY);
        settings_registry::resetToDefaults();
        for (uint8_t i = 0; i < G_NUM_PATTERNS; i++)
            g_patterns[i]->resetDefaults();
        resetEffectStackDefaults();
        for (uint8_t i = 0; i < G_NUM_EFFECTS; i++)
            g_effects[i]->resetDefaults();
        settings_registry::clearNvs();
        cfg_->motorStopped = true;
        cfg_->escPulseUs = kStopPulseUs;
        xSemaphoreGive(cfgMutex_);
        motor_->stop();
        if (configCb_) configCb_();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* req) {
        ESP_LOGI("web", "POST /api/reboot — restarting");
        BaseType_t started = xTaskCreate(
            delayedRebootTask,
            "webReboot",
            kRebootTaskStackBytes,
            nullptr,
            kRebootTaskPriority,
            nullptr
        );
        if (started != pdPASS) {
            req->send(500, "application/json", "{\"error\":\"reboot task failed\"}");
            return;
        }
        req->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/image", HTTP_POST,
        [this](AsyncWebServerRequest* req) {
            if (!imgBuffer_ || imgReceived_ != imgExpected_) {
                free(imgBuffer_);
                imgBuffer_ = nullptr;
                req->send(400, "application/json", "{\"error\":\"incomplete upload\"}");
                return;
            }
            if (imageCb_) imageCb_(imgBuffer_, imgWidth_, imgHeight_);
            free(imgBuffer_);
            imgBuffer_ = nullptr;
            if (configCb_) configCb_();
            req->send(200, "application/json", "{\"ok\":true}");
        },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                free(imgBuffer_);
                imgBuffer_ = nullptr;

                if (req->hasParam("w") && req->hasParam("h")) {
                    imgWidth_  = req->getParam("w")->value().toInt();
                    imgHeight_ = req->getParam("h")->value().toInt();
                } else {
                    return;
                }
                imgExpected_ = (size_t)imgWidth_ * imgHeight_ * 3;
                if (total != imgExpected_ || imgExpected_ > kMaxImageSize) return;
                imgBuffer_ = (uint8_t*)malloc(imgExpected_);
                if (!imgBuffer_) return;
                imgReceived_ = 0;
            }
            if (!imgBuffer_) return;
            size_t room = imgExpected_ - imgReceived_;
            size_t copy = len < room ? len : room;
            memcpy(imgBuffer_ + imgReceived_, data, copy);
            imgReceived_ += copy;
        }
    );

    server_.on("/js/image-processor.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", IMAGE_PROCESSOR_JS);
    });

    server_.on("/js/settings.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", SETTINGS_JS);
    });
}
