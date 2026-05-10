#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "framebuffer.h"
#include "hal_spi_leds.h"
#include "hall_sensor.h"
#include "slice_scheduler.h"
#include "motor.h"
#include "web/web_server.h"

#include "patterns/pattern.h"
#include "patterns/solid.h"
#include "patterns/rainbow.h"
#include "patterns/text.h"
#include "patterns/scanner.h"

// --- Globals ---
static Config         cfg;
static Framebuffer    fb;
static LedDriver      leds;
static HallSensor     hall;
static SliceScheduler scheduler;
static Motor          motor;
static PovWebServer   webServer;

static SolidPattern   solidPattern;
static RainbowPattern rainbowPattern;
static TextPattern    textPattern;
static ScannerPattern scannerPattern;

static constexpr uint8_t NUM_PATTERNS = 4;
static Pattern* patterns[NUM_PATTERNS] = { &solidPattern, &rainbowPattern, &textPattern, &scannerPattern };

static volatile bool configDirty = false;

// --- Callbacks ---
static void onConfigChanged() {
    configDirty = true;
}

// --- Pattern task (Core 1, lower priority than render) ---
static void patternTaskFunc(void*) {
    bool wasDirectMode = false;

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
            motor.setPulseUs(cfg.escPulseUs);
        }

        uint8_t pi = cfg.activePattern;
        if (pi >= NUM_PATTERNS) pi = 0;
        patterns[pi]->generate(fb, cfg, millis());

        // When not spinning, push slice 0 directly to the strip
        uint32_t lastHall = hall.lastTriggerMs();
        bool spinning = (lastHall > 0) && ((millis() - lastHall) < 500);
        if (!spinning) {
            if (!wasDirectMode) {
                Serial.printf("[pattern] direct mode: pushing slice 0 to strip (%u leds)\n", fb.numLeds());
                wasDirectMode = true;
            }
            const Pixel* data = fb.getSlice(0);
            leds.sendSlice(data, fb.numLeds());
        } else if (wasDirectMode) {
            Serial.println("[pattern] scheduler mode: hall sensor active");
            wasDirectMode = false;
        }

        // ~30 fps for animated patterns
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

// --- Hall sensor polling task (Core 1) ---
static void hallPollTaskFunc(void*) {
    for (;;) {
        if (hall.consumeNewRotation()) {
            scheduler.onNewRotation();
        }
        vTaskDelay(1); // 1 tick ≈ 1ms — fast enough for rotation detection
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("POV Display starting...");

    // Load saved config
    cfg.loadFromNvs();

    // Init LED driver
    if (!leds.init(PIN_LED_CLK, PIN_LED_MOSI, cfg.spiClockMhz, MAX_LEDS)) {
        Serial.println("SPI init failed!");
        return;
    }
    leds.allOff(cfg.numLeds);

    // Init framebuffer
    size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t fbNeed  = (size_t)cfg.numSlices * cfg.numLeds * sizeof(Pixel) * 2;
    Serial.printf("DMA free: %u bytes, FB needs: %u bytes (%ux%u)\n",
                  dmaFree, fbNeed, cfg.numSlices, cfg.numLeds);
    if (!fb.init(cfg.numSlices, cfg.numLeds)) {
        Serial.println("Framebuffer alloc failed!");
        return;
    }

    // Init hall sensor
    hall.init(PIN_HALL);

    // Init slice scheduler (creates render task on Core 1)
    scheduler.init(&fb, &leds, &hall);
    scheduler.setNumSlices(cfg.numSlices);
    scheduler.setPhaseOffset(cfg.phaseOffset);
    scheduler.start();

    // Init motor
    motor.init(PIN_ESC);

    // Generate initial frame
    uint8_t pi = cfg.activePattern;
    if (pi >= NUM_PATTERNS) pi = 0;
    patterns[pi]->generate(fb, cfg, millis());

    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP("POV-Display");
    Serial.printf("softAP: %s\n", apOk ? "OK" : "FAILED");
    Serial.printf("AP IP: %s  MAC: %s\n",
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPmacAddress().c_str());

    // Start web server
    Serial.println("Starting web server...");
    webServer.init(&cfg, &hall, &fb, &motor);
    webServer.onConfigChange(onConfigChanged);
    Serial.println("Web server started on port 80");

    // Create pattern task on Core 1
    xTaskCreatePinnedToCore(patternTaskFunc, "pattern", 8192, nullptr, 10, nullptr, 1);

    // Create hall polling task on Core 1
    xTaskCreatePinnedToCore(hallPollTaskFunc, "hallpoll", 2048, nullptr, 20, nullptr, 1);

    Serial.println("Ready.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
