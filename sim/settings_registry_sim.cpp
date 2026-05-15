#include "../src/settings_registry.h"
#include "../src/config.h"
#include "../src/framebuffer.h"
#include "timing.h"
#include "renderer.h"

// Pointers to sim state populated by sim_bridge.cpp before use.
static TimingState* s_ts  = nullptr;
static Config*      s_cfg = nullptr;
static Framebuffer* s_fb  = nullptr;

void sim_registry_bind(TimingState* ts, Config* cfg, Framebuffer* fb) {
    s_ts  = ts;
    s_cfg = cfg;
    s_fb  = fb;
}

// ── SimOnly accessors ──────────────────────────────────────────────────────

void sim_apply_geometry(int numLeds) {
    if (numLeds <= 0) return;
    float ledTotal = numLeds * (LED_SIZE_MM + LED_GAP_MM);
    renderer_set_hub_fraction(HUB_RADIUS_MM / ledTotal);
    renderer_set_gap_fraction(LED_GAP_MM / (LED_SIZE_MM + LED_GAP_MM));
}

static int32_t get_rpmJitter()    { return s_ts ? (int32_t)(s_ts->rpmJitter * 10.0f) : 0; }
static void    set_rpmJitter(int32_t v) { if (s_ts) s_ts->rpmJitter = v / 10.0f; }
static int32_t get_hallJitter()   { return s_ts ? (int32_t)s_ts->hallJitterUs : 0; }
static void    set_hallJitter(int32_t v) { if (s_ts) s_ts->hallJitterUs = (float)v; }
static int32_t get_hallMiss()     { return s_ts ? (int32_t)s_ts->hallMissRate : 0; }
static void    set_hallMiss(int32_t v) { if (s_ts) s_ts->hallMissRate = (float)v; }
static int32_t get_timerDrift()   { return s_ts ? (int32_t)s_ts->timerDriftPpm : 0; }
static void    set_timerDrift(int32_t v) { if (s_ts) s_ts->timerDriftPpm = (float)v; }
static int32_t get_patternLag()   { return s_ts ? (int32_t)s_ts->patternLagMs : 0; }
static void    set_patternLag(int32_t v) { if (s_ts) s_ts->patternLagMs = (float)v; }
static int32_t get_displayHz()    { return s_ts ? (int32_t)s_ts->displayHz : 60; }
static void    set_displayHz(int32_t v) { if (s_ts) s_ts->displayHz = (float)v; }

static float g_simSpeed = 1.0f;
static int32_t get_simSpeed()   { return (int32_t)(g_simSpeed * 10.0f); }
static void    set_simSpeed(int32_t v) { g_simSpeed = v / 10.0f; }

static bool g_showOverruns    = true;
static bool g_showSliceGrid   = false;
static bool g_showHallMarker  = true;
static int32_t get_showOverruns()   { return g_showOverruns ? 1 : 0; }
static void    set_showOverruns(int32_t v) {
    g_showOverruns = v != 0;
    renderer_set_show_overruns(g_showOverruns);
}
static int32_t get_showSliceGrid()  { return g_showSliceGrid ? 1 : 0; }
static void    set_showSliceGrid(int32_t v) {
    g_showSliceGrid = v != 0;
    renderer_set_show_slice_grid(g_showSliceGrid);
}
static int32_t get_showHallMarker() { return g_showHallMarker ? 1 : 0; }
static void    set_showHallMarker(int32_t v) {
    g_showHallMarker = v != 0;
    renderer_set_show_hall_marker(g_showHallMarker);
}

// ── Enum option tables ─────────────────────────────────────────────────────

static const ParamOption kDisplayHzOpts[] = {
    {"60 Hz", 60}, {"120 Hz", 120}, {"144 Hz", 144}, {"240 Hz", 240}
};

// ── Registry ──────────────────────────────────────────────────────────────

const Setting g_sim_settings[] = {
    // timing distortion
    { "rpmJitter",   "RPM jitter",    "hardware", "timing", Scope::SimOnly, ParamType::Int,
       0, 0, 200, 10, nullptr, 0,
       get_rpmJitter, set_rpmJitter, nullptr, nullptr, nullptr },
    { "hallJitterUs","Hall jitter µs","hardware", "timing", Scope::SimOnly, ParamType::Int,
       0, 0, 500, 1, nullptr, 0,
       get_hallJitter, set_hallJitter, nullptr, nullptr, nullptr },
    { "hallMissRate","Hall miss %",   "hardware", "timing", Scope::SimOnly, ParamType::Int,
       0, 0, 50, 1, nullptr, 0,
       get_hallMiss, set_hallMiss, nullptr, nullptr, nullptr },
    { "timerDrift",  "Timer drift ppm","hardware", "timing", Scope::SimOnly, ParamType::Int,
       0, 0, 1000, 1, nullptr, 0,
       get_timerDrift, set_timerDrift, nullptr, nullptr, nullptr },
    { "patternLag",  "Pattern lag ms","hardware", "timing", Scope::SimOnly, ParamType::Int,
       0, 0, 100, 1, nullptr, 0,
       get_patternLag, set_patternLag, nullptr, nullptr, nullptr },
    // playback
    { "displayHz",   "Display Hz",    "hardware", "playback", Scope::SimOnly, ParamType::Enum,
       60, 0, 240, 1, kDisplayHzOpts, 4,
       get_displayHz, set_displayHz, nullptr, nullptr, nullptr },
    { "simSpeed",    "Sim speed",     "hardware", "playback", Scope::SimOnly, ParamType::Int,
       10, 1, 50, 10, nullptr, 0,
       get_simSpeed, set_simSpeed, nullptr, nullptr, nullptr },
    // overlays
    { "showOverruns",   "Show overruns",    "hardware", "overlays", Scope::SimOnly, ParamType::Bool,
       1, 0, 1, 1, nullptr, 0,
       get_showOverruns, set_showOverruns, nullptr, nullptr, nullptr },
    { "showSliceGrid",  "Show slice grid",  "hardware", "overlays", Scope::SimOnly, ParamType::Bool,
       0, 0, 1, 1, nullptr, 0,
       get_showSliceGrid, set_showSliceGrid, nullptr, nullptr, nullptr },
    { "showHallMarker", "Show hall marker", "hardware", "overlays", Scope::SimOnly, ParamType::Bool,
       1, 0, 1, 1, nullptr, 0,
       get_showHallMarker, set_showHallMarker, nullptr, nullptr, nullptr },
};
const uint16_t G_NUM_SIM_SETTINGS = sizeof(g_sim_settings) / sizeof(g_sim_settings[0]);

float sim_settings_get_sim_speed() { return g_simSpeed; }
