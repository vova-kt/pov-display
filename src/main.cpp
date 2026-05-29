#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "framebuffer.h"
#include "hal_spi_leds.h"
#include "hall_sensor.h"
#include "hall_timing_source.h"
#include "slice_scheduler.h"
#include "motor.h"
#include "web/web_server.h"

#include "effect.h"
#include "settings_registry.h"
#include "patterns/pattern.h"
#include "patterns/registry.h"
#include "patterns/image.h"

// --- Globals ---
static Config         cfg;
static Framebuffer    fb;
static LedDriver      leds;
static HallSensor     hall;
static HallTimingSource hallTiming(&hall);
static SliceScheduler scheduler;
static Motor          motor;
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
            leds.setReversed(cfg.stripReversed);
            motor.setPulseUs(cfg.escPulseUs);
        }

        uint8_t pi = cfg.activePattern;
        if (pi >= G_NUM_PATTERNS) pi = 0;
        uint32_t now = millis();
        g_patterns[pi]->generate(fb, cfg, now);

        EffectState effectState;
        applyEffects(effectState, fb, now);
        fb.swap();
        scheduler.setPhaseOffset(cfg.phaseOffset + effectState.sliceOffset);

        // When not spinning, push slice 0 directly to the strip
        uint32_t lastHall = hallTiming.lastTriggerMs();
        bool spinning = (lastHall > 0) && ((millis() - lastHall) < 500);
        if (!spinning) {
            if (!wasDirectMode) {
                Serial.printf("[pattern] direct mode: pushing slice 0 to strip (%u leds)\n", fb.numLeds());
                wasDirectMode = true;
            }
            const Pixel* data = fb.getSlice(0);

            if (frameCount % 90 == 0) {
                uint32_t nonZero = 0;
                for (uint16_t i = 0; i < fb.numLeds(); i++) {
                    if (data[i].red || data[i].green || data[i].blue) nonZero++;
                }
                Serial.printf("[dbg] frame=%lu pat=%u('%s') bright=%u leds=%u nonZero=%lu\n",
                              frameCount, pi, g_patterns[pi]->name(), cfg.brightness, fb.numLeds(), nonZero);
                if (fb.numLeds() > 0) {
                    const Pixel& p0 = data[0];
                    const Pixel& pM = data[fb.numLeds() / 2];
                    Serial.printf("[dbg] px[0]={br=0x%02X r=%u g=%u b=%u} px[%u]={br=0x%02X r=%u g=%u b=%u}\n",
                                  p0.brightness, p0.red, p0.green, p0.blue,
                                  fb.numLeds() / 2,
                                  pM.brightness, pM.red, pM.green, pM.blue);
                }
            }

            leds.sendSlice(data, fb.numLeds());
        } else if (wasDirectMode) {
            Serial.println("[pattern] scheduler mode: hall sensor active");
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
    for (;;) {
        if (hallTiming.consumeNewRotation()) {
            trigCount++;
            uint32_t now = millis();
            if (now - lastLogMs > 2000) {
                Serial.printf("[hall] trig #%lu  rpm=%lu  period=%luus  pin=%d\n",
                              trigCount, hall.rpm(), hall.rotationPeriodUs(),
                              digitalRead(PIN_HALL));
                lastLogMs = now;
            }
            scheduler.onNewRotation();
        }
        vTaskDelay(1);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("POV Display starting...");

    // Init motor first — ESC times out if it doesn't see 1000µs within ~2s of power-on
    Serial.printf("[motor] init on GPIO%u\n", PIN_ESC);
    motor.init(PIN_ESC);
    motor.arm();

    // Load saved config
    settings_registry::init(&cfg);
    settings_registry::loadFromNvs();
    loadPatternsFromNvs();
    loadEffectsFromNvs();

    Serial.println("=== CONFIG DUMP ===");
    Serial.printf("  numLeds=%u  numSlices=%u\n", cfg.numLeds, cfg.numSlices);
    Serial.printf("  brightness=%u  maxBrightness=%u\n", cfg.brightness, cfg.maxBrightness);
    Serial.printf("  activePattern=%u  color=#%02X%02X%02X\n", cfg.activePattern, cfg.colorR, cfg.colorG, cfg.colorB);
    Serial.printf("  numArms=%u  targetHz=%u  escPulseUs=%u\n", cfg.numArms, cfg.targetHz, cfg.escPulseUs);
    Serial.printf("  spiClockMhz=%u  mirror=%d  radialBalance=%d\n", cfg.spiClockMhz, cfg.mirrorPattern, cfg.radialBalance);
    Serial.printf("  phaseOffset=%d\n", cfg.phaseOffset);
    Serial.println("=== PIN MAP ===");
    Serial.printf("  SPI CLK=GPIO%u  MOSI=GPIO%u\n", PIN_LED_CLK, PIN_LED_MOSI);
    Serial.printf("  Hall=GPIO%u  ESC=GPIO%u\n", PIN_HALL, PIN_ESC);

    // Init LED driver
    Serial.printf("[spi] init: clk=GPIO%u mosi=GPIO%u clock=%uMHz maxLeds=%u\n",
                  PIN_LED_CLK, PIN_LED_MOSI, cfg.spiClockMhz, MAX_LEDS);
    if (!leds.init(PIN_LED_CLK, PIN_LED_MOSI, cfg.spiClockMhz, MAX_LEDS)) {
        Serial.println("[spi] INIT FAILED!");
        return;
    }
    Serial.println("[spi] init OK");
    leds.allOff(cfg.numLeds);
    Serial.printf("[spi] allOff sent (%u leds)\n", cfg.numLeds);
    leds.recomputeScale(cfg.numLeds, cfg.radialBalance);
    leds.setReversed(cfg.stripReversed);

    // Init framebuffer
    size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t fbNeed  = (size_t)cfg.numSlices * cfg.numLeds * sizeof(Pixel) * 2;
    Serial.printf("[fb] DMA free: %u bytes, need: %u bytes (%ux%u)\n",
                  dmaFree, fbNeed, cfg.numSlices, cfg.numLeds);
    if (!fb.init(cfg.numSlices, cfg.numLeds)) {
        Serial.println("[fb] ALLOC FAILED!");
        return;
    }
    Serial.println("[fb] init OK");

    // Init hall sensor
    Serial.printf("[hall] init on GPIO%u (INPUT_PULLUP, FALLING edge)\n", PIN_HALL);
    hall.init(PIN_HALL);
    Serial.printf("[hall] pin state after init: %d\n", digitalRead(PIN_HALL));

    // Init slice scheduler (creates render task)
    scheduler.init(&fb, &leds, &hallTiming);
    scheduler.setNumSlices(cfg.numSlices);
    scheduler.setPhaseOffset(cfg.phaseOffset);
    scheduler.setMirror(cfg.mirrorPattern);
    scheduler.start();
    Serial.println("[scheduler] started");

    // Apply saved throttle now that config is loaded
    uint32_t targetRpm = (uint32_t)cfg.targetHz * 60 / cfg.numArms;
    Serial.printf("[motor] targetHz=%u numArms=%u -> targetRpm=%lu\n", cfg.targetHz, cfg.numArms, targetRpm);
    Serial.printf("[motor] applying escPulseUs=%u (1000=stop, 1100=slow, 2000=full)\n", cfg.escPulseUs);
    motor.setPulseUs(cfg.escPulseUs);

    // Generate initial frame
    uint8_t pi = cfg.activePattern;
    if (pi >= G_NUM_PATTERNS) pi = 0;
    Serial.printf("[pattern] initial generate: pattern=%u '%s'\n", pi, g_patterns[pi]->name());
    g_patterns[pi]->generate(fb, cfg, millis());
    fb.swap();

    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP("POV-Display", "756Rhebpv!");
    Serial.printf("softAP: %s\n", apOk ? "OK" : "FAILED");
    Serial.printf("AP IP: %s  MAC: %s\n",
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPmacAddress().c_str());

    // Start web server
    Serial.println("Starting web server...");
    webServer.init(&cfg, &hall, &fb, &motor);
    webServer.onConfigChange(onConfigChanged);
    webServer.onImageUpload(onImageUpload);
    Serial.println("Web server started on port 80");

    xTaskCreate(patternTaskFunc, "pattern", 8192, nullptr, 10, nullptr);
    xTaskCreate(hallPollTaskFunc, "hallpoll", 2048, nullptr, 20, nullptr);

    Serial.printf("Free heap: %u  Free DMA: %u\n",
                  ESP.getFreeHeap(),
                  heap_caps_get_free_size(MALLOC_CAP_DMA));
    Serial.println("Ready.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
