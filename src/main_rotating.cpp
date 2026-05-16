#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "framebuffer.h"
#include "hal_spi_leds.h"
#include "slice_scheduler.h"
#include "comm/wifi_timing.h"
#include "comm/config_relay.h"

#include "effect.h"
#include "settings_registry.h"
#include "patterns/pattern.h"
#include "patterns/registry.h"
#include "patterns/image.h"

static Config              cfg;
static Framebuffer         fb;
static LedDriver           leds;
static WifiTimingSource    timing;
static SliceScheduler      scheduler;
static ConfigRelayReceiver configRx;

static volatile bool configDirty = false;

static void onConfigChanged() {
    configDirty = true;
}

static void patternTaskFunc(void*) {
    bool wasDirectMode = false;

    for (;;) {
        if (configDirty) {
            configDirty = false;

            if (cfg.numLeds != fb.numLeds() || cfg.numSlices != fb.numSlices()) {
                size_t need = (size_t)cfg.numSlices * cfg.numLeds * sizeof(Pixel) * 2;
                size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
                Serial.printf("[pattern] resize fb: %ux%u -> %ux%u (need=%u, dma_free=%u)\n",
                              fb.numSlices(), fb.numLeds(), cfg.numSlices, cfg.numLeds,
                              need, dmaFree);
                scheduler.stop();
                bool ok = fb.resize(cfg.numSlices, cfg.numLeds);
                if (ok) {
                    Serial.printf("[pattern] resize OK (heap=%u)\n", ESP.getFreeHeap());
                    scheduler.setNumSlices(cfg.numSlices);
                } else {
                    Serial.printf("[pattern] resize FAILED — keeping %ux%u (heap=%u)\n",
                                  fb.numSlices(), fb.numLeds(), ESP.getFreeHeap());
                }
                scheduler.start();
            }

            scheduler.setPhaseOffset(cfg.phaseOffset);
            scheduler.setMirror(cfg.mirrorPattern);
            leds.recomputeScale(cfg.numLeds, cfg.radialBalance);

#ifdef MAX_BRIGHTNESS_CAP
            if (cfg.maxBrightness > MAX_BRIGHTNESS_CAP)
                cfg.maxBrightness = MAX_BRIGHTNESS_CAP;
            if (cfg.brightness > cfg.maxBrightness)
                cfg.brightness = cfg.maxBrightness;
#endif
        }

        uint8_t pi = cfg.activePattern;
        if (pi >= G_NUM_PATTERNS) pi = 0;
        uint32_t now = millis();
        g_patterns[pi]->generate(fb, cfg, now);

        EffectState effectState;
        applyEffects(effectState, fb, now);
        fb.swap();
        scheduler.setPhaseOffset(cfg.phaseOffset + effectState.sliceOffset);

        uint32_t lastTrig = timing.lastTriggerMs();
        bool spinning = (lastTrig > 0) && ((millis() - lastTrig) < 500);
        if (!spinning) {
            if (!wasDirectMode) {
                Serial.printf("[pattern] direct mode: pushing slice 0 to strip (%u leds)\n", fb.numLeds());
                wasDirectMode = true;
            }
            const Pixel* data = fb.getSlice(0);
            leds.sendSlice(data, fb.numLeds());
        } else if (wasDirectMode) {
            Serial.println("[pattern] scheduler mode: wifi timing active");
            wasDirectMode = false;
        }

        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

static void timingTask(void*) {
    for (;;) {
        timing.poll();
        if (timing.consumeNewRotation()) {
            scheduler.onNewRotation();
        }
        vTaskDelay(1);
    }
}

static void configRecvTask(void*) {
    for (;;) {
        configRx.poll();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("POV Rotating MCU starting...");

    settings_registry::init(&cfg);
    settings_registry::loadFromNvs();
    loadPatternsFromNvs();
    loadEffectsFromNvs();

#ifdef MAX_BRIGHTNESS_CAP
    if (cfg.maxBrightness > MAX_BRIGHTNESS_CAP)
        cfg.maxBrightness = MAX_BRIGHTNESS_CAP;
    if (cfg.brightness > cfg.maxBrightness)
        cfg.brightness = cfg.maxBrightness;
#endif

    if (!leds.init(PIN_LED_CLK, PIN_LED_MOSI, cfg.spiClockMhz, MAX_LEDS)) {
        Serial.println("SPI init failed!");
        return;
    }
    leds.allOff(cfg.numLeds);
    leds.recomputeScale(cfg.numLeds, cfg.radialBalance);

    size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t fbNeed  = (size_t)cfg.numSlices * cfg.numLeds * sizeof(Pixel) * 2;
    Serial.printf("DMA free: %u bytes, FB needs: %u bytes (%ux%u)\n",
                  dmaFree, fbNeed, cfg.numSlices, cfg.numLeds);
    if (!fb.init(cfg.numSlices, cfg.numLeds)) {
        Serial.println("Framebuffer alloc failed!");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin("POV-Display", "756Rhebpv!");
    Serial.print("Connecting to AP");
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.printf("\nConnected, IP: %s\n", WiFi.localIP().toString().c_str());

    timing.init();
    configRx.init();
    configRx.onConfigChange(onConfigChanged);

    scheduler.init(&fb, &leds, &timing);
    scheduler.setNumSlices(cfg.numSlices);
    scheduler.setPhaseOffset(cfg.phaseOffset);
    scheduler.setMirror(cfg.mirrorPattern);
    scheduler.start();

    uint8_t pi = cfg.activePattern;
    if (pi >= G_NUM_PATTERNS) pi = 0;
    g_patterns[pi]->generate(fb, cfg, millis());
    fb.swap();

    xTaskCreate(patternTaskFunc, "pattern", 8192, nullptr, 10, nullptr);
    xTaskCreate(timingTask, "timing", 2048, nullptr, 20, nullptr);
    xTaskCreate(configRecvTask, "cfgrecv", 4096, nullptr, 5, nullptr);

    Serial.println("Rotating MCU ready.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
