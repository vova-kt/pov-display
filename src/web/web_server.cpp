#include "web_server.h"
#include "web_ui.h"
#include <ArduinoJson.h>

static String bodyBuffer;

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
        doc["numLeds"]       = cfg_->numLeds;
        doc["numSlices"]     = cfg_->numSlices;
        doc["brightness"]    = cfg_->brightness;
        doc["maxBrightness"] = cfg_->maxBrightness;
        doc["phaseOffset"]   = cfg_->phaseOffset;
        doc["activePattern"] = cfg_->activePattern;
        doc["colorR"]        = cfg_->colorR;
        doc["colorG"]        = cfg_->colorG;
        doc["colorB"]        = cfg_->colorB;
        doc["text"]          = cfg_->text;
        doc["numArms"]       = cfg_->numArms;
        doc["targetHz"]      = cfg_->targetHz;
        doc["escPulseUs"]    = cfg_->escPulseUs;
        doc["mirrorPattern"] = cfg_->mirrorPattern;
        xSemaphoreGive(cfgMutex_);

        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    server_.on("/api/config", HTTP_POST,
        [this](AsyncWebServerRequest* req) {
            Serial.printf("WebServer: POST /api/config (%u bytes)\n", bodyBuffer.length());
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyBuffer);
            if (err) {
                req->send(400, "application/json", "{\"error\":\"bad json\"}");
                bodyBuffer = "";
                return;
            }
            JsonObject obj = doc.as<JsonObject>();
            xSemaphoreTake(cfgMutex_, portMAX_DELAY);

            if (obj["numLeds"].is<uint16_t>())       cfg_->numLeds       = obj["numLeds"].as<uint16_t>();
            if (obj["numSlices"].is<uint16_t>()) {
                uint16_t s = obj["numSlices"].as<uint16_t>();
                if (s == 90 || s == 180 || s == 270 || s == 360)
                    cfg_->numSlices = s;
            }
            if (obj["brightness"].is<uint8_t>()) {
                uint8_t b = obj["brightness"].as<uint8_t>();
                cfg_->brightness = (b > cfg_->maxBrightness) ? cfg_->maxBrightness : b;
            }
            if (obj["phaseOffset"].is<int16_t>()) {
                int16_t p = obj["phaseOffset"].as<int16_t>();
                if (p == -90 || p == 0 || p == 90 || p == 180)
                    cfg_->phaseOffset = p;
            }
            if (obj["activePattern"].is<uint8_t>())  cfg_->activePattern = obj["activePattern"].as<uint8_t>();
            if (obj["colorR"].is<uint8_t>())         cfg_->colorR        = obj["colorR"].as<uint8_t>();
            if (obj["colorG"].is<uint8_t>())         cfg_->colorG        = obj["colorG"].as<uint8_t>();
            if (obj["colorB"].is<uint8_t>())         cfg_->colorB        = obj["colorB"].as<uint8_t>();
            if (obj["numArms"].is<uint8_t>())         cfg_->numArms       = obj["numArms"].as<uint8_t>();
            if (obj["targetHz"].is<uint8_t>())       cfg_->targetHz      = obj["targetHz"].as<uint8_t>();
            if (obj["escPulseUs"].is<uint16_t>())    cfg_->escPulseUs    = obj["escPulseUs"].as<uint16_t>();
            if (obj["mirrorPattern"].is<bool>())     cfg_->mirrorPattern = obj["mirrorPattern"].as<bool>();
            if (obj["text"].is<const char*>()) {
                strlcpy(cfg_->text, obj["text"].as<const char*>(), sizeof(cfg_->text));
            }

            xSemaphoreGive(cfgMutex_);
            bodyBuffer = "";

            if (configCb_) configCb_();

            req->send(200, "application/json", "{\"ok\":true}");
        },
        nullptr,
        [](AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t) {
            if (index == 0) bodyBuffer = "";
            bodyBuffer += String((char*)data, len);
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
        cfg_->saveToNvs();
        xSemaphoreGive(cfgMutex_);
        req->send(200, "application/json", "{\"ok\":true}");
    });
}
