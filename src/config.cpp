#include "config.h"
#include <Preferences.h>
#include <cstring>

// NVS namespace — all config keys live under this single partition namespace
static const char* NVS_NS = "pov";

// Load config from NVS flash. Each getter's second arg is the default if key is missing,
// so a fresh ESP32 (or after NVS erase) falls back to the struct's compiled defaults.
void Config::loadFromNvs() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return;  // true = read-only

    // Display settings
    numLeds       = prefs.getUShort("numLeds",    numLeds);
    numSlices     = prefs.getUShort("numSlices",   numSlices);
    brightness    = prefs.getUChar("brightness",   brightness);
    maxBrightness = prefs.getUChar("maxBright",    maxBrightness);
    phaseOffset   = prefs.getShort("phaseOff",     phaseOffset);

    // Pattern settings
    activePattern = prefs.getUChar("pattern",      activePattern);
    colorR        = prefs.getUChar("colorR",       colorR);
    colorG        = prefs.getUChar("colorG",       colorG);
    colorB        = prefs.getUChar("colorB",       colorB);

    // Motor / hardware
    escPulseUs    = prefs.getUShort("escPulse",    escPulseUs);
    spiClockMhz   = prefs.getUChar("spiClk",      spiClockMhz);

    // Text pattern string — fall back to "HELLO" if never saved
    size_t len = prefs.getString("text", text, sizeof(text));
    if (len == 0) strcpy(text, "HELLO");

    prefs.end();
}

// Persist current config to NVS flash. Called explicitly via "Save to Flash" button
// in the web UI — not on every change, to limit flash wear.
void Config::saveToNvs() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return;  // false = read-write

    // Display settings
    prefs.putUShort("numLeds",    numLeds);
    prefs.putUShort("numSlices",  numSlices);
    prefs.putUChar("brightness",  brightness);
    prefs.putUChar("maxBright",   maxBrightness);
    prefs.putShort("phaseOff",    phaseOffset);

    // Pattern settings
    prefs.putUChar("pattern",     activePattern);
    prefs.putUChar("colorR",      colorR);
    prefs.putUChar("colorG",      colorG);
    prefs.putUChar("colorB",      colorB);

    // Motor / hardware
    prefs.putUShort("escPulse",   escPulseUs);
    prefs.putUChar("spiClk",      spiClockMhz);
    prefs.putString("text",       text);

    prefs.end();
}
