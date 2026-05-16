#include "web_server.h"
#include "web_ui.h"
#include "image_processor_js.h"
#include "settings_js.h"
#include "../effect.h"
#include "../settings_registry.h"
#include "../patterns/registry.h"
#include <ArduinoJson.h>

void PovWebServer::init(Config* cfg, HallSensor* hall, Framebuffer* fb, Motor* motor) {
    cfg_   = cfg;
    hall_  = hall;
    fb_    = fb;
    motor_ = motor;
    cfgMutex_ = xSemaphoreCreateMutex();
    Serial.printf("WebServer: mutex=%s\n", cfgMutex_ ? "OK" : "FAILED");
    setupRoutes();
    Serial.println("WebServer: routes registered");
    server_.begin();
    Serial.printf("WebServer: listening on port 80 (free heap: %u)\n", ESP.getFreeHeap());
}

void PovWebServer::setupRoutes() {
    server_.onNotFound([](AsyncWebServerRequest* req) {
        Serial.printf("WebServer: 404 %s %s\n", req->methodToString(), req->url().c_str());
        req->send(404, "text/plain", "Not found");
    });

    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        Serial.printf("WebServer: GET / from %s\n", req->client()->remoteIP().toString().c_str());
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
            Serial.printf("WebServer: POST /api/config (%u bytes)\n", bodyBuffer_.length());
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
        doc["rpm"]      = hall_->rpm();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["uptime"]   = millis();
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
