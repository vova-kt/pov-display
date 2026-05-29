#pragma once
#include <cstdint>

struct TimingState {
    float rpm            = 1800.0f;
    float rpmJitter      = 0.0f;
    float hallJitterUs   = 0.0f;
    float hallMissRate   = 0.0f;
    float timerDriftPpm  = 0.0f;
    float patternLagMs   = 0.0f;
    float spiClockMhz    = 20.0f;
    float displayHz      = 60.0f;
    int   numLeds        = 40;
    int   numSlices      = 360;
    float patternFps     = 60.0f;

    float armAngle         = 0.0f;
    int   frameAge         = 0;
    float lastPatternGenMs = -1e9f;
};

struct FrameResult {
    float periodMs;
    float actualRpm;
    float sliceIntervalUs;
    float spiTransferUs;
    float headroomUs;
    float hallOffsetAngle;
    float armAngle;
    float armSweep;
    bool  hallMissed;
    bool  hasOverruns;
    int   frameAge;
    bool  shouldGenerate;
};

void  timing_init(TimingState& ts);
FrameResult timing_frame(TimingState& ts, float dtMs, float simTimeMs);
float timing_pattern_gen_ms(const TimingState& ts);
