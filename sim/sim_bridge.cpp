#include <emscripten.h>
#include <cstring>
#include "framebuffer.h"
#include "config.h"
#include "patterns/solid.h"
#include "patterns/rainbow.h"
#include "patterns/scanner.h"
#include "patterns/text.h"

static Framebuffer fb;
static Config cfg;

static SolidPattern   solidPattern;
static RainbowPattern rainbowPattern;
static ScannerPattern scannerPattern;
static TextPattern    textPattern;

static Pattern* patterns[] = {
    &solidPattern,
    &rainbowPattern,
    &textPattern,
    &scannerPattern,
};
static constexpr uint8_t NUM_PATTERNS = sizeof(patterns) / sizeof(patterns[0]);

extern "C" {

EMSCRIPTEN_KEEPALIVE
bool sim_init(uint16_t numSlices, uint16_t numLeds) {
    return fb.init(numSlices, numLeds);
}

EMSCRIPTEN_KEEPALIVE
bool sim_resize(uint16_t numSlices, uint16_t numLeds) {
    return fb.resize(numSlices, numLeds);
}

EMSCRIPTEN_KEEPALIVE
void sim_generate(uint8_t patternIndex, uint32_t timeMs) {
    if (patternIndex >= NUM_PATTERNS) return;
    patterns[patternIndex]->generate(fb, cfg, timeMs);
}

EMSCRIPTEN_KEEPALIVE
uint8_t* sim_get_framebuffer() {
    return (uint8_t*)fb.getSlice(0);
}

EMSCRIPTEN_KEEPALIVE
uint16_t sim_num_slices() { return fb.numSlices(); }

EMSCRIPTEN_KEEPALIVE
uint16_t sim_num_leds() { return fb.numLeds(); }

EMSCRIPTEN_KEEPALIVE
void sim_set_brightness(uint8_t b) { cfg.brightness = b; }

EMSCRIPTEN_KEEPALIVE
void sim_set_color(uint8_t r, uint8_t g, uint8_t b) {
    cfg.colorR = r; cfg.colorG = g; cfg.colorB = b;
}

EMSCRIPTEN_KEEPALIVE
void sim_set_phase_offset(int16_t offset) { cfg.phaseOffset = offset; }

EMSCRIPTEN_KEEPALIVE
void sim_set_text(const char* t) {
    strncpy(cfg.text, t, sizeof(cfg.text) - 1);
    cfg.text[sizeof(cfg.text) - 1] = '\0';
}

EMSCRIPTEN_KEEPALIVE
uint8_t sim_num_patterns() { return NUM_PATTERNS; }

EMSCRIPTEN_KEEPALIVE
const char* sim_pattern_name(uint8_t index) {
    if (index >= NUM_PATTERNS) return "";
    return patterns[index]->name();
}

}
