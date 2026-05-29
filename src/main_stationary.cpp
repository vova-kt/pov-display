#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hall_sensor.h"
#include "motor.h"
#include "motor_control.h"
#include "web/web_server.h"
#include "settings_registry.h"
#include "comm/wifi_timing.h"
#include "comm/config_relay.h"

#include "effect.h"
#include "patterns/registry.h"

static Config               cfg;
static HallSensor           hall;
static Motor                motor;
static MotorSpeedController motorControl;
static PovWebServer         webServer;
static WifiTimingBroadcaster timingTx;
static ConfigRelayBroadcaster configTx;

static volatile bool configDirty = false;

static void onConfigChanged() {
    configDirty = true;
}

static void onConfigRelay(const char* json, size_t len) {
    configTx.send(json, len);
}

static void hallBroadcastTask(void*) {
    for (;;) {
        if (hall.consumeNewRotation()) {
            timingTx.broadcast(hall.rotationPeriodUs());
        }
        vTaskDelay(1);
    }
}

static void configTask(void*) {
    uint8_t lastTargetHz = cfg.targetHz;
    uint8_t lastNumArms = cfg.numArms;
    bool lastStopped = true;
    uint32_t lastControlMs = 0;

    for (;;) {
        if (configDirty)
            configDirty = false;

        bool stopped = cfg.motorStopped;
        uint8_t targetHz = cfg.targetHz;
        uint8_t numArms = cfg.numArms;

        if (stopped) {
            if (!lastStopped || cfg.escPulseUs != kStopPulseUs) {
                motorControl.stop();
                cfg.escPulseUs = motorControl.pulseUs();
                motor.stop();
            }
            lastStopped = true;
        } else {
            uint32_t now = millis();
            bool targetChanged = targetHz != lastTargetHz || numArms != lastNumArms;

            if (lastStopped || !motorControl.running()) {
                motorControl.start(targetHz, numArms);
                cfg.escPulseUs = motorControl.pulseUs();
                motor.setPulseUs(cfg.escPulseUs);
                lastControlMs = now;
            } else if (targetChanged) {
                motorControl.setTarget(targetHz, numArms);
            }

            if (now - lastControlMs >= kMotorControlIntervalMs) {
                uint32_t measuredRpm = freshHallRpm(hall.rpm(), hall.lastTriggerMs(), now);
                uint16_t nextPulse = motorControl.update(measuredRpm);
                if (nextPulse != cfg.escPulseUs) {
                    cfg.escPulseUs = nextPulse;
                    motor.setPulseUs(nextPulse);
                }
                lastControlMs = now;
            }

            lastStopped = false;
        }

        lastTargetHz = targetHz;
        lastNumArms = numArms;
        vTaskDelay(pdMS_TO_TICKS(kMotorControlTaskDelayMs));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("POV Stationary MCU starting...");

    settings_registry::init(&cfg);
    settings_registry::loadFromNvs();
    loadPatternsFromNvs();
    loadEffectsFromNvs();

    hall.init(PIN_HALL);
    motor.init(PIN_ESC);

    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP("POV-Display", "756Rhebpv!");
    Serial.printf("softAP: %s\n", apOk ? "OK" : "FAILED");
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());

    timingTx.init();
    configTx.init();

    webServer.init(&cfg, &hall, nullptr, &motor);
    webServer.onConfigChange(onConfigChanged);
    webServer.onConfigRelay(onConfigRelay);
    Serial.println("Web server started on port 80");

    xTaskCreate(hallBroadcastTask, "hallbc", 2048, nullptr, 20, nullptr);
    xTaskCreate(configTask, "config", 4096, nullptr, 10, nullptr);

    Serial.println("Stationary MCU ready.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
