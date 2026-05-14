#include "timing.h"
#include <cmath>
#include <cstdlib>

static constexpr float TAU = 6.283185307179586f;

static float rand_uniform() {
    float r;
    do { r = (float)rand() / (float)RAND_MAX; } while (r == 0.0f);
    return r;
}

static float gauss_random() {
    return sqrtf(-2.0f * logf(rand_uniform())) * cosf(TAU * rand_uniform());
}

void timing_init(TimingState& ts) {
    ts.armAngle = 0.0f;
    ts.frameAge = 0;
    ts.lastPatternGenMs = -1e9f;
}

float timing_pattern_gen_ms(const TimingState& ts) {
    float baseMs = (ts.numSlices * ts.numLeds * 100e-9f) * 1000.0f;
    return baseMs + ts.patternLagMs;
}

static float spi_transfer_us(const TimingState& ts) {
    int bytes = 4 + ts.numLeds * 4 + (ts.numLeds + 15) / 16;
    int bits = bytes * 8;
    return (float)bits / (ts.spiClockMhz * 1e6f) * 1e6f;
}

FrameResult timing_frame(TimingState& ts, float dtMs, float simTimeMs) {
    float jitterFactor = 1.0f + gauss_random() * (ts.rpmJitter / 100.0f);
    float actualRpm = ts.rpm * fmaxf(0.1f, jitterFactor);
    float periodMs = 60000.0f / actualRpm;

    float hallOffsetUs = gauss_random() * ts.hallJitterUs;
    bool  hallMissed = rand_uniform() < ts.hallMissRate;

    float patternGenTime = timing_pattern_gen_ms(ts);
    float patternInterval = 1000.0f / ts.patternFps;
    bool  shouldGenerate = false;

    if (simTimeMs - ts.lastPatternGenMs >= patternInterval) {
        if (patternGenTime > patternInterval) {
            ts.frameAge++;
        } else {
            ts.frameAge = 0;
            ts.lastPatternGenMs = simTimeMs;
            shouldGenerate = true;
        }
    }

    float sliceIntervalUs = (periodMs * 1000.0f) / ts.numSlices;
    float spiUs = spi_transfer_us(ts);
    bool  hasOverruns = spiUs > sliceIntervalUs;

    float hallOffsetAngle = (hallOffsetUs / (periodMs * 1000.0f)) * TAU;

    float armSweep = 0.0f;
    if (dtMs > 0.0f) {
        armSweep = fminf((dtMs / periodMs) * TAU, TAU);
        ts.armAngle = fmodf(ts.armAngle + armSweep, TAU);
    }

    FrameResult r;
    r.periodMs        = periodMs;
    r.actualRpm       = actualRpm;
    r.sliceIntervalUs = sliceIntervalUs;
    r.spiTransferUs   = spiUs;
    r.headroomUs      = sliceIntervalUs - spiUs;
    r.hallOffsetAngle = hallOffsetAngle;
    r.armAngle        = ts.armAngle;
    r.armSweep        = armSweep;
    r.hallMissed      = hallMissed;
    r.hasOverruns     = hasOverruns;
    r.frameAge        = ts.frameAge;
    r.shouldGenerate  = shouldGenerate;
    return r;
}
