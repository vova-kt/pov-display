#include <emscripten.h>
#include <cstring>
#include <cstdio>
#include "framebuffer.h"
#include "config.h"
#include "effect.h"
#include "timing.h"
#include "renderer.h"
#include "effect_phase.h"
#include "settings_registry.h"
#include "settings_registry_sim.h"
#include "patterns/registry.h"
#include "patterns/image.h"
#include <ArduinoJson.h>

static Framebuffer fb;
static Config cfg;
static TimingState ts;
static FrameResult lastFrame;
static SimEffectPhase effectPhase;

static void render_current_frame() {
    int16_t savedPhase = cfg.phaseOffset;
    cfg.phaseOffset = effectPhase.phaseOffset(cfg.phaseOffset);
    renderer_render(fb, cfg,
                    lastFrame.armAngle, lastFrame.armSweep,
                    lastFrame.hallOffsetAngle, lastFrame.hasOverruns,
                    fb.numSlices());
    cfg.phaseOffset = savedPhase;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
bool sim_init(uint16_t numSlices, uint16_t numLeds) {
    (void)numSlices;
    (void)numLeds;
    settings_registry::init(&cfg);
    ts.numSlices = cfg.numSlices;
    ts.numLeds = cfg.numLeds;
    timing_init(ts);
    effectPhase.reset();
    bool ok = fb.init(cfg.numSlices, cfg.numLeds);
    sim_registry_bind(&ts, &cfg, &fb);
    sim_apply_geometry(cfg.numLeds);
    return ok;
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
void sim_set_mirror_pattern(bool m) { cfg.mirrorPattern = m; }

EMSCRIPTEN_KEEPALIVE
void sim_set_radial_balance(bool v) { cfg.radialBalance = v; }

static Param* textParam(const char* paramKey) {
    int idx = g_pattern_index("text");
    if (idx < 0) return nullptr;
    return g_patterns[idx]->findParam(paramKey);
}

EMSCRIPTEN_KEEPALIVE
void sim_set_text(const char* t) {
    Param* p = textParam("text");
    if (p) param_set_text(*p, t);
}

EMSCRIPTEN_KEEPALIVE
void sim_set_text_mode(uint8_t m) {
    Param* p = textParam("mode");
    if (p && m <= 3) p->value = m;
}

EMSCRIPTEN_KEEPALIVE
void sim_set_text_delay(uint16_t ms) {
    Param* p = textParam("delayMs");
    if (p) param_set_int(*p, ms);
}

EMSCRIPTEN_KEEPALIVE
uint8_t sim_num_patterns() { return G_NUM_PATTERNS; }

EMSCRIPTEN_KEEPALIVE
const char* sim_pattern_name(uint8_t index) {
    if (index >= G_NUM_PATTERNS) return "";
    return g_patterns[index]->name();
}

EMSCRIPTEN_KEEPALIVE
uint16_t sim_num_slices() { return fb.numSlices(); }

EMSCRIPTEN_KEEPALIVE
uint16_t sim_num_leds() { return fb.numLeds(); }

// --- Renderer ---

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
void sim_timing_set_display_hz(float v) { ts.displayHz = v; }

EMSCRIPTEN_KEEPALIVE
void sim_frame(float dtMs, float simTimeMs, uint8_t patternIndex) {
    lastFrame = timing_frame(ts, dtMs, simTimeMs);

    if (lastFrame.shouldGenerate && patternIndex < G_NUM_PATTERNS) {
        g_patterns[patternIndex]->generate(fb, cfg, (uint32_t)simTimeMs);

        EffectState effectState;
        applyEffects(effectState, fb, (uint32_t)simTimeMs);
        effectPhase.update(effectState);
        fb.swap();

        render_current_frame();
    } else {
        render_current_frame();
    }
}

EMSCRIPTEN_KEEPALIVE float sim_get_actual_rpm()       { return lastFrame.actualRpm; }
EMSCRIPTEN_KEEPALIVE float sim_get_slice_interval_us() { return lastFrame.sliceIntervalUs; }
EMSCRIPTEN_KEEPALIVE float sim_get_spi_transfer_us()  { return lastFrame.spiTransferUs; }
EMSCRIPTEN_KEEPALIVE float sim_get_headroom_us()      { return lastFrame.headroomUs; }
EMSCRIPTEN_KEEPALIVE int   sim_get_frame_age()        { return lastFrame.frameAge; }
EMSCRIPTEN_KEEPALIVE float sim_get_pattern_gen_ms()   { return timing_pattern_gen_ms(ts); }
EMSCRIPTEN_KEEPALIVE bool  sim_get_hall_missed()      { return lastFrame.hallMissed; }
EMSCRIPTEN_KEEPALIVE bool  sim_get_has_overruns()     { return lastFrame.hasOverruns; }

EMSCRIPTEN_KEEPALIVE
bool sim_load_image(uint8_t* rgbData, uint16_t width, uint16_t height) {
    return g_image_pattern()->loadImage(rgbData, width, height);
}

// --- Settings JSON API ---

static char s_settingsJsonBuf[6144];

EMSCRIPTEN_KEEPALIVE
const char* sim_get_settings_json() {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    settings_registry::toJson(root, Scope::SimOnly);
    serializeJson(doc, s_settingsJsonBuf, sizeof(s_settingsJsonBuf));
    return s_settingsJsonBuf;
}

EMSCRIPTEN_KEEPALIVE
bool sim_apply_settings_json(const char* json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;
    settings_registry::applyJson(doc.as<JsonObjectConst>(), Scope::SimOnly);
    // Propagate numLeds/numSlices changes to framebuffer and timing state.
    if ((int)cfg.numLeds != ts.numLeds || (int)cfg.numSlices != ts.numSlices) {
        ts.numLeds   = cfg.numLeds;
        ts.numSlices = cfg.numSlices;
        fb.resize(cfg.numSlices, cfg.numLeds);
        sim_apply_geometry(cfg.numLeds);
    }
    renderer_set_num_arms(cfg.numArms);
    ts.spiClockMhz = cfg.spiClockMhz;
    // RPM is derived from targetHz x 60 / numArms
    ts.rpm = (float)cfg.targetHz * 60.0f / (float)cfg.numArms;
    return true;
}

EMSCRIPTEN_KEEPALIVE
float sim_get_sim_speed() { return sim_settings_get_sim_speed(); }

}
