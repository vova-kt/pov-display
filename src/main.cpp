#include <Arduino.h>
#include <WiFi.h>

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

static constexpr uint8_t NUM_PATTERNS = 3;
static Pattern* patterns[NUM_PATTERNS] = { &solidPattern, &rainbowPattern, &textPattern };

static volatile bool configDirty = false;

// --- Callbacks ---
static void onConfigChanged() {
    configDirty = true;
}

// --- Pattern task (Core 1, lower priority than render) ---
static void patternTaskFunc(void*) {
    for (;;) {
        if (configDirty) {
            configDirty = false;

            // Resize framebuffer if LED count or slice count changed
            if (cfg.numLeds != fb.numLeds() || cfg.numSlices != fb.numSlices()) {
                scheduler.stop();
                fb.resize(cfg.numSlices, cfg.numLeds);
                scheduler.setNumSlices(cfg.numSlices);
                scheduler.start();
            }

            scheduler.setPhaseOffset(cfg.phaseOffset);
            motor.setPulseUs(cfg.escPulseUs);
        }

        uint8_t pi = cfg.activePattern;
        if (pi >= NUM_PATTERNS) pi = 0;
        patterns[pi]->generate(fb, cfg, millis());

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
    WiFi.softAP("POV-Display");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    // Start web server
    webServer.init(&cfg, &hall, &fb, &motor);
    webServer.onConfigChange(onConfigChanged);

    // Create pattern task on Core 1
    xTaskCreatePinnedToCore(patternTaskFunc, "pattern", 8192, nullptr, 10, nullptr, 1);

    // Create hall polling task on Core 1
    xTaskCreatePinnedToCore(hallPollTaskFunc, "hallpoll", 2048, nullptr, 20, nullptr, 1);

    Serial.println("Ready.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
