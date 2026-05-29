#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "log_tags.h"
#include "framebuffer.h"
#include "hal_spi_leds.h"
#include "hall_sensor.h"
#include "hall_timing_source.h"
#include "slice_scheduler.h"
#include "motor.h"
#include "motor_control.h"
#include "web/web_server.h"

#include "effect.h"
#include "settings_registry.h"
#include "patterns/pattern.h"
#include "patterns/registry.h"
#include "patterns/image.h"

LOG_TAG(main);

// --- Globals ---
static Config         cfg;
static Framebuffer    fb;
static LedDriver      leds;
static HallSensor     hall;
static HallTimingSource hallTiming(&hall);
static SliceScheduler scheduler;
static Motor          motor;
static MotorSpeedController motorControl;
static PovWebServer   webServer;

static volatile bool configDirty = false;

// --- Callbacks ---
static void onConfigChanged() {
    configDirty = true;
}

static void onImageUpload(const uint8_t* rgbData, uint16_t width, uint16_t height) {
    g_image_pattern()->loadImage(rgbData, width, height);
    int idx = g_pattern_index("image");
    if (idx >= 0) cfg.activePattern = (uint8_t)idx;
    configDirty = true;
}

// --- Pattern task (Core 1, lower priority than render) ---
static void patternTaskFunc(void*) {
    bool wasDirectMode = false;
    uint32_t frameCount = 0;

    for (;;) {
        if (configDirty) {
            configDirty = false;

            // Resize framebuffer if LED count or slice count changed
            if (cfg.numLeds != fb.numLeds() || cfg.numSlices != fb.numSlices()) {
                size_t need = (size_t)cfg.numSlices * cfg.numLeds * sizeof(Pixel) * 2;
                size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
                ESP_LOGI("pattern", "resize fb: %ux%u -> %ux%u (need=%u, dma_free=%u)",
                         fb.numSlices(), fb.numLeds(), cfg.numSlices, cfg.numLeds,
                         need, dmaFree);
                scheduler.beginFramebufferResize();
                bool ok = fb.resize(cfg.numSlices, cfg.numLeds);
                if (ok) {
                    ESP_LOGI("pattern", "resize OK (heap=%u)", ESP.getFreeHeap());
                    scheduler.setNumSlices(cfg.numSlices);
                } else {
                    ESP_LOGE("pattern", "resize FAILED — keeping %ux%u (heap=%u)",
                             fb.numSlices(), fb.numLeds(), ESP.getFreeHeap());
                    cfg.numSlices = fb.numSlices();
                    cfg.numLeds = fb.numLeds();
                    scheduler.setNumSlices(cfg.numSlices);
                }
                scheduler.endFramebufferResize();
            }

            scheduler.setPhaseOffset(cfg.phaseOffset);
            scheduler.setMirror(cfg.mirrorPattern);
            leds.recomputeScale(cfg.numLeds, cfg.radialBalance);
            leds.setReversed(cfg.stripReversed);
        }

        uint8_t pi = cfg.activePattern;
        if (pi >= G_NUM_PATTERNS) pi = 0;
        uint32_t now = millis();
        g_patterns[pi]->generate(fb, cfg, now);

        EffectState effectState;
        applyEffects(effectState, fb, now);
        fb.swap();
        scheduler.setPhaseOffset(cfg.phaseOffset + effectState.sliceOffset);

        // When not spinning, ask the render task to push slice 0
        uint32_t lastHall = hallTiming.lastTriggerMs();
        bool spinning = freshHallRpm(hall.rpm(), lastHall, millis()) > 0;
        if (!spinning) {
            if (!wasDirectMode) {
                ESP_LOGI("pattern", "direct mode: pushing slice 0 to strip (%u leds)", fb.numLeds());
                wasDirectMode = true;
            }

            if (frameCount % 90 == 0) {
                const Pixel* data = fb.getSlice(0);
                uint32_t nonZero = 0;
                for (uint16_t i = 0; i < fb.numLeds(); i++) {
                    if (data[i].red || data[i].green || data[i].blue) nonZero++;
                }
                ESP_LOGD("pattern", "frame=%lu pat=%u('%s') bright=%u leds=%u nonZero=%lu",
                         frameCount, pi, g_patterns[pi]->name(), cfg.brightness, fb.numLeds(), nonZero);
                if (fb.numLeds() > 0) {
                    const Pixel& p0 = data[0];
                    const Pixel& pM = data[fb.numLeds() / 2];
                    ESP_LOGD("pattern", "px[0]={br=0x%02X r=%u g=%u b=%u} px[%u]={br=0x%02X r=%u g=%u b=%u}",
                             p0.brightness, p0.red, p0.green, p0.blue,
                             fb.numLeds() / 2,
                             pM.brightness, pM.red, pM.green, pM.blue);
                }
            }

            scheduler.requestDirectPush();
        } else if (wasDirectMode) {
            ESP_LOGI("pattern", "scheduler mode: hall sensor active");
            wasDirectMode = false;
        }

        frameCount++;
        // ~30 fps for animated patterns
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

// --- Hall sensor polling task (Core 1) ---
static void hallPollTaskFunc(void*) {
    uint32_t trigCount = 0;
    uint32_t lastLogMs = 0;
    uint32_t lastIdleLogMs = 0;
    for (;;) {
        if (hallTiming.consumeNewRotation()) {
            trigCount++;
            uint32_t now = millis();
            if (now - lastLogMs > 2000) {
                uint32_t expectedRpm = targetRefreshHzToRpm(cfg.targetHz, cfg.numArms);
                uint32_t actualRpm = hall.rpm();
                int32_t rpmError = (int32_t)expectedRpm - (int32_t)actualRpm;
                ESP_LOGI("hall", "rpm expected=%lu actual=%lu expected-actual=%ld  trig #%lu  period=%luus  pin=%d",
                         (unsigned long)expectedRpm, (unsigned long)actualRpm, (long)rpmError,
                         (unsigned long)trigCount, (unsigned long)hall.rotationPeriodUs(),
                         digitalRead(PIN_HALL));
                lastLogMs = now;
            }
            scheduler.onNewRotation();
        }

        uint32_t now = millis();
        uint32_t lastHall = hall.lastTriggerMs();
        bool spinning = freshHallRpm(hall.rpm(), lastHall, now) > 0;
        if (!spinning && (now - lastIdleLogMs > 1000)) {
            ESP_LOGD("hall", "idle: pin=%d  lastTrig=%lums ago  rpm=%lu  period=%luus  trigs=%lu",
                     digitalRead(PIN_HALL),
                     lastHall ? (now - lastHall) : 0,
                     hall.rpm(), hall.rotationPeriodUs(),
                     trigCount);
            lastIdleLogMs = now;
        }

        vTaskDelay(1);
    }
}

// --- Closed-loop motor control task ---
static void motorControlTaskFunc(void*) {
    uint8_t lastTargetHz = cfg.targetHz;
    uint8_t lastNumArms = cfg.numArms;
    bool lastStopped = true;
    uint32_t lastControlMs = 0;

    for (;;) {
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
                    ESP_LOGI("motor", "ctl: target=%lu rpm measured=%lu pulse=%u",
                            motorControl.targetRpm(), measuredRpm, nextPulse);
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
    ESP_LOGI(TAG, "POV Display starting...");

    // Init motor first — ESC times out if it doesn't see 1000µs within ~2s of power-on
    ESP_LOGI("motor", "init on GPIO%u", PIN_ESC);
    motor.init(PIN_ESC);
    motor.arm();

    // Load saved config and apply log level
    settings_registry::init(&cfg);
    settings_registry::loadFromNvs();
    esp_log_level_set("*", (esp_log_level_t)cfg.logLevel);
    pov_log_apply_exclusions();
    loadPatternsFromNvs();
    loadEffectsFromNvs();

    ESP_LOGI(TAG, "=== CONFIG ===");
    ESP_LOGI(TAG, "  numLeds=%u  numSlices=%u", cfg.numLeds, cfg.numSlices);
    ESP_LOGI(TAG, "  brightness=%u  maxBrightness=%u", cfg.brightness, cfg.maxBrightness);
    ESP_LOGI(TAG, "  activePattern=%u  color=#%02X%02X%02X", cfg.activePattern, cfg.colorR, cfg.colorG, cfg.colorB);
    ESP_LOGI(TAG, "  numArms=%u (build)  targetHz=%u  escPulseUs=%u", cfg.numArms, cfg.targetHz, cfg.escPulseUs);
    ESP_LOGI(TAG, "  spiClockMhz=%u  mirror=%d  radialBalance=%d", cfg.spiClockMhz, cfg.mirrorPattern, cfg.radialBalance);
    ESP_LOGI(TAG, "  phaseOffset=%d  logLevel=%u", cfg.phaseOffset, cfg.logLevel);
    ESP_LOGI(TAG, "=== PINS ===");
    ESP_LOGI(TAG, "  SPI CLK=GPIO%u  MOSI=GPIO%u", PIN_LED_CLK, PIN_LED_MOSI);
    ESP_LOGI(TAG, "  Hall=GPIO%u  ESC=GPIO%u", PIN_HALL, PIN_ESC);

    // Init LED driver
    ESP_LOGI("spi", "init: clk=GPIO%u mosi=GPIO%u clock=%uMHz maxLeds=%u",
             PIN_LED_CLK, PIN_LED_MOSI, cfg.spiClockMhz, MAX_LEDS);
    if (!leds.init(PIN_LED_CLK, PIN_LED_MOSI, cfg.spiClockMhz, MAX_LEDS)) {
        ESP_LOGE("spi", "INIT FAILED!");
        return;
    }
    ESP_LOGI("spi", "init OK");
    leds.allOff(cfg.numLeds);
    ESP_LOGI("spi", "allOff sent (%u leds)", cfg.numLeds);
    leds.recomputeScale(cfg.numLeds, cfg.radialBalance);
    leds.setReversed(cfg.stripReversed);

    // Init framebuffer
    size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t fbNeed  = (size_t)cfg.numSlices * cfg.numLeds * sizeof(Pixel) * 2;
    ESP_LOGI("fb", "DMA free: %u bytes, need: %u bytes (%ux%u)",
             dmaFree, fbNeed, cfg.numSlices, cfg.numLeds);
    if (!fb.init(cfg.numSlices, cfg.numLeds)) {
        ESP_LOGE("fb", "ALLOC FAILED!");
        return;
    }
    ESP_LOGI("fb", "init OK");

    // Init hall sensor
    ESP_LOGI("hall", "init on GPIO%u (INPUT_PULLUP, FALLING edge)", PIN_HALL);
    hall.init(PIN_HALL);
    ESP_LOGI("hall", "pin state after init: %d", digitalRead(PIN_HALL));

    // Init slice scheduler (creates render task)
    scheduler.init(&fb, &leds, &hallTiming);
    scheduler.setNumSlices(cfg.numSlices);
    scheduler.setPhaseOffset(cfg.phaseOffset);
    scheduler.setMirror(cfg.mirrorPattern);
    scheduler.start();
    ESP_LOGI("scheduler", "started");

    // Boot stays stopped; the motor control task owns ESC pulses after Start.
    ESP_LOGI("motor", "targetHz=%u numArms=%u targetRpm=%lu escPulseUs=%u",
             cfg.targetHz, cfg.numArms,
             targetRefreshHzToRpm(cfg.targetHz, cfg.numArms),
             cfg.escPulseUs);
    motor.setPulseUs(cfg.escPulseUs);

    // Generate initial frame
    uint8_t pi = cfg.activePattern;
    if (pi >= G_NUM_PATTERNS) pi = 0;
    ESP_LOGI("pattern", "initial generate: pattern=%u '%s'", pi, g_patterns[pi]->name());
    g_patterns[pi]->generate(fb, cfg, millis());
    fb.swap();

    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP("POV-Display", "756Rhebpv!");
    ESP_LOGI(TAG, "softAP: %s", apOk ? "OK" : "FAILED");
    ESP_LOGI(TAG, "AP IP: %s  MAC: %s",
             WiFi.softAPIP().toString().c_str(),
             WiFi.softAPmacAddress().c_str());

    // Start web server
    ESP_LOGI("web", "starting...");
    webServer.init(&cfg, &hall, &fb, &motor);
    webServer.onConfigChange(onConfigChanged);
    webServer.onImageUpload(onImageUpload);
    ESP_LOGI("web", "started on port 80");

    xTaskCreate(patternTaskFunc, "pattern", 8192, nullptr, 10, nullptr);
    xTaskCreate(hallPollTaskFunc, "hallpoll", 2048, nullptr, 20, nullptr);
    xTaskCreate(motorControlTaskFunc, "motorctl", 2048, nullptr, 9, nullptr);

    ESP_LOGI(TAG, "Free heap: %u  Free DMA: %u",
             ESP.getFreeHeap(),
             heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "Ready.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
