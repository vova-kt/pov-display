#include <emscripten.h>
#include <cstring>
#include "framebuffer.h"
#include "config.h"
#include "timing.h"
#include "renderer.h"
#include "patterns/solid.h"
#include "patterns/rainbow.h"
#include "patterns/scanner.h"
#include "patterns/text.h"

static Framebuffer fb;
static Config cfg;
static TimingState ts;
static FrameResult lastFrame;

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
    ts.numSlices = numSlices;
    ts.numLeds = numLeds;
    timing_init(ts);
    return fb.init(numSlices, numLeds);
}

EMSCRIPTEN_KEEPALIVE
bool sim_resize(uint16_t numSlices, uint16_t numLeds) {
    ts.numSlices = numSlices;
    ts.numLeds = numLeds;
    return fb.resize(numSlices, numLeds);
}

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

EMSCRIPTEN_KEEPALIVE
uint16_t sim_num_slices() { return fb.numSlices(); }

EMSCRIPTEN_KEEPALIVE
uint16_t sim_num_leds() { return fb.numLeds(); }

EMSCRIPTEN_KEEPALIVE
bool sim_renderer_init() {
    return renderer_init();
}

EMSCRIPTEN_KEEPALIVE
void sim_renderer_resize(int w, int h) { renderer_resize(w, h); }

EMSCRIPTEN_KEEPALIVE
void sim_renderer_set_hub_frac(float f) { renderer_set_hub_fraction(f); }

EMSCRIPTEN_KEEPALIVE
void sim_renderer_set_gap_frac(float f) { renderer_set_gap_fraction(f); }

EMSCRIPTEN_KEEPALIVE
void sim_renderer_set_show_overruns(bool v) { renderer_set_show_overruns(v); }

EMSCRIPTEN_KEEPALIVE
void sim_renderer_set_show_hall_marker(bool v) { renderer_set_show_hall_marker(v); }

EMSCRIPTEN_KEEPALIVE
void sim_renderer_set_show_slice_grid(bool v) { renderer_set_show_slice_grid(v); }

EMSCRIPTEN_KEEPALIVE
void sim_renderer_set_num_arms(int n) { renderer_set_num_arms(n); }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_rpm(float v) { ts.rpm = v; }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_rpm_jitter(float v) { ts.rpmJitter = v; }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_hall_jitter(float v) { ts.hallJitterUs = v; }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_hall_miss(float v) { ts.hallMissRate = v; }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_timer_drift(float v) { ts.timerDriftPpm = v; }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_pattern_lag(float v) { ts.patternLagMs = v; }

EMSCRIPTEN_KEEPALIVE
void sim_timing_set_spi_clock(float v) { ts.spiClockMhz = v; }

EMSCRIPTEN_KEEPALIVE
void sim_frame(float dtMs, float simTimeMs, uint8_t patternIndex) {
    lastFrame = timing_frame(ts, dtMs, simTimeMs);

    if (lastFrame.shouldGenerate && patternIndex < NUM_PATTERNS) {
        patterns[patternIndex]->generate(fb, cfg, (uint32_t)simTimeMs);
    }

    renderer_render(fb, cfg,
                    lastFrame.armAngle, lastFrame.armSweep,
                    lastFrame.hallOffsetAngle, lastFrame.hasOverruns,
                    fb.numSlices());
}

EMSCRIPTEN_KEEPALIVE float sim_get_actual_rpm()       { return lastFrame.actualRpm; }
EMSCRIPTEN_KEEPALIVE float sim_get_slice_interval_us() { return lastFrame.sliceIntervalUs; }
EMSCRIPTEN_KEEPALIVE float sim_get_spi_transfer_us()  { return lastFrame.spiTransferUs; }
EMSCRIPTEN_KEEPALIVE float sim_get_headroom_us()      { return lastFrame.headroomUs; }
EMSCRIPTEN_KEEPALIVE int   sim_get_frame_age()        { return lastFrame.frameAge; }
EMSCRIPTEN_KEEPALIVE float sim_get_pattern_gen_ms()   { return timing_pattern_gen_ms(ts); }
EMSCRIPTEN_KEEPALIVE bool  sim_get_hall_missed()      { return lastFrame.hallMissed; }
EMSCRIPTEN_KEEPALIVE bool  sim_get_has_overruns()     { return lastFrame.hasOverruns; }

}
