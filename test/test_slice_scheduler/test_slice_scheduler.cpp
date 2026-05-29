#include <unity.h>
#include <cstring>
#include "framebuffer.h"
#include "slice_scheduler.h"

// --- LedDriver stub: records sendSlice calls ---

struct SendRecord {
    Pixel pixels[MAX_LEDS];
    uint16_t count;
};

static SendRecord g_sends[16];
static uint8_t    g_sendCount;

bool LedDriver::init(uint8_t, uint8_t, uint8_t, uint16_t) { return true; }
void LedDriver::allOff(uint16_t) {}
void LedDriver::recomputeScale(uint16_t, bool) {}
uint16_t LedDriver::buildFrame(const Pixel*, uint16_t) { return 0; }

void LedDriver::sendSlice(const Pixel* pixels, uint16_t count) {
    if (g_sendCount < 16) {
        memcpy(g_sends[g_sendCount].pixels, pixels, count * sizeof(Pixel));
        g_sends[g_sendCount].count = count;
        g_sendCount++;
    }
}

// --- Stub TimingSource ---

class StubTiming : public TimingSource {
public:
    bool     consumeNewRotation() override { return false; }
    uint32_t rotationPeriodUs() const override { return periodUs; }
    uint32_t lastTriggerMs()    const override { return 0; }
    uint32_t periodUs = 0;
};

// --- Test fixtures ---

static Framebuffer    fb;
static LedDriver      leds;
static StubTiming     timing;
static SliceScheduler sched;

static void fillSlice(uint16_t slice, uint8_t r, uint8_t g, uint8_t b) {
    Pixel* row = fb.backSlice(slice);
    for (uint16_t i = 0; i < fb.numLeds(); i++)
        row[i] = {(uint8_t)(0xE0 | 16), b, g, r};
}

void setUp() {
    fb = Framebuffer();
    TEST_ASSERT_TRUE(fb.init(8, 4));
    sched = SliceScheduler();
    sched.init(&fb, &leds, &timing);
    sched.setNumSlices(8);
    sched.setMirror(false);
    sched.start();
    g_sendCount = 0;
    memset(g_sends, 0, sizeof(g_sends));
}

void tearDown() {
    fb.release();
}

// --- Timer-driven slice tests ---

void test_timer_slice_sends_current_slice() {
    fillSlice(0, 255, 0, 0);
    fillSlice(1, 0, 255, 0);
    fb.swap();

    sched.onNewRotation(); // won't do much without real timing
    // Manually simulate: currentSlice=0, numSlices=8 → timer path
    // processNotification reads currentSlice_ which onNewRotation set to 0
    // but timing stub returns 0 period, so onNewRotation exits early.
    // Instead, set state directly via setNumSlices + setPhaseOffset:
    sched.setNumSlices(8);
    sched.setPhaseOffset(0);

    // Simulate timer-driven state: currentSlice < numSlices
    // We need to trigger onNewRotation with a valid period.
    timing.periodUs = 100000; // 100ms = 600 RPM
    sched.onNewRotation();

    // currentSlice_ is now 0, numSlices_ is 8 → timer path
    bool handled = sched.processNotification();
    TEST_ASSERT_TRUE(handled);
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);
    TEST_ASSERT_EQUAL_UINT16(4, g_sends[0].count);
    TEST_ASSERT_EQUAL_UINT8(255, g_sends[0].pixels[0].red);
    TEST_ASSERT_EQUAL_UINT8(0, g_sends[0].pixels[0].green);
}

void test_timer_slice_advances() {
    fillSlice(0, 255, 0, 0);
    fillSlice(1, 0, 255, 0);
    fillSlice(2, 0, 0, 255);
    fb.swap();

    timing.periodUs = 100000;
    sched.onNewRotation();

    sched.processNotification(); // slice 0
    sched.processNotification(); // slice 1
    TEST_ASSERT_EQUAL_UINT8(2, g_sendCount);
    TEST_ASSERT_EQUAL_UINT8(0, g_sends[1].pixels[0].red);
    TEST_ASSERT_EQUAL_UINT8(255, g_sends[1].pixels[0].green);

    sched.processNotification(); // slice 2
    TEST_ASSERT_EQUAL_UINT8(3, g_sendCount);
    TEST_ASSERT_EQUAL_UINT8(0, g_sends[2].pixels[0].red);
    TEST_ASSERT_EQUAL_UINT8(255, g_sends[2].pixels[0].blue);
}

void test_timer_slice_stops_at_numslices() {
    timing.periodUs = 100000;
    sched.onNewRotation();

    for (int i = 0; i < 8; i++)
        sched.processNotification();
    TEST_ASSERT_EQUAL_UINT8(8, g_sendCount);

    // 9th call: currentSlice >= numSlices, no directPush → returns false
    bool handled = sched.processNotification();
    TEST_ASSERT_FALSE(handled);
    TEST_ASSERT_EQUAL_UINT8(8, g_sendCount);
}

void test_timer_slice_mirror() {
    sched.setMirror(true);
    fillSlice(7, 255, 0, 0);  // mirror of slice 0 when numSlices=8
    fillSlice(6, 0, 255, 0);  // mirror of slice 1
    fb.swap();

    timing.periodUs = 100000;
    sched.onNewRotation();

    sched.processNotification(); // slice 0 → reads fb slice 7
    TEST_ASSERT_EQUAL_UINT8(255, g_sends[0].pixels[0].red);

    sched.processNotification(); // slice 1 → reads fb slice 6
    TEST_ASSERT_EQUAL_UINT8(255, g_sends[1].pixels[0].green);
}

// --- Direct push tests ---

static void drainTimerSlices() {
    timing.periodUs = 100000;
    sched.onNewRotation();
    for (int i = 0; i < 8; i++)
        sched.processNotification();
    g_sendCount = 0;
}

void test_direct_push_sends_first_slice() {
    fillSlice(0, 42, 100, 200);
    fb.swap();

    drainTimerSlices();

    sched.requestDirectPush();
    bool handled = sched.processNotification();
    TEST_ASSERT_TRUE(handled);
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);
    TEST_ASSERT_EQUAL_UINT16(4, g_sends[0].count);
    TEST_ASSERT_EQUAL_UINT8(42, g_sends[0].pixels[0].red);
    TEST_ASSERT_EQUAL_UINT8(100, g_sends[0].pixels[0].green);
    TEST_ASSERT_EQUAL_UINT8(200, g_sends[0].pixels[0].blue);
}

void test_direct_push_cycles_through_slices() {
    for (uint16_t i = 0; i < 8; i++)
        fillSlice(i, i * 30, 0, 0);
    fb.swap();

    drainTimerSlices();

    for (int i = 0; i < 8; i++) {
        sched.requestDirectPush();
        sched.processNotification();
    }
    TEST_ASSERT_EQUAL_UINT8(8, g_sendCount);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT_EQUAL_UINT8(i * 30, g_sends[i].pixels[0].red);
}

void test_direct_push_wraps_around() {
    for (uint16_t i = 0; i < 8; i++)
        fillSlice(i, i * 30, 0, 0);
    fb.swap();

    drainTimerSlices();

    // Push all 8 slices
    for (int i = 0; i < 8; i++) {
        sched.requestDirectPush();
        sched.processNotification();
    }
    g_sendCount = 0;

    // Next push wraps to slice 0
    sched.requestDirectPush();
    sched.processNotification();
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);
    TEST_ASSERT_EQUAL_UINT8(0, g_sends[0].pixels[0].red);
}

void test_direct_push_clears_flag() {
    drainTimerSlices();

    sched.requestDirectPush();
    sched.processNotification();
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);

    // Second call without another requestDirectPush → no send
    bool handled = sched.processNotification();
    TEST_ASSERT_FALSE(handled);
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);
}

void test_timer_slice_takes_priority_over_direct_push() {
    fillSlice(0, 255, 0, 0);
    fillSlice(1, 0, 255, 0);
    fb.swap();

    timing.periodUs = 100000;
    sched.onNewRotation();

    // Set direct push while timer slices are pending
    sched.requestDirectPush();

    // Timer path should win (currentSlice < numSlices)
    sched.processNotification();
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);
    TEST_ASSERT_EQUAL_UINT8(255, g_sends[0].pixels[0].red);

    // Consume remaining timer slices
    for (int i = 1; i < 8; i++)
        sched.processNotification();

    // Now directPush_ is still set → next call handles it
    g_sendCount = 0;
    bool handled = sched.processNotification();
    TEST_ASSERT_TRUE(handled);
    TEST_ASSERT_EQUAL_UINT8(1, g_sendCount);
}

void test_no_notification_returns_false() {
    // currentSlice >= numSlices (consume all), no directPush
    timing.periodUs = 100000;
    sched.onNewRotation();
    for (int i = 0; i < 8; i++)
        sched.processNotification();

    bool handled = sched.processNotification();
    TEST_ASSERT_FALSE(handled);
}

void test_phase_offset_shifts_start_slice() {
    sched.setPhaseOffset(3);
    for (uint16_t i = 0; i < 8; i++)
        fillSlice(i, i * 30, 0, 0);
    fb.swap();

    timing.periodUs = 100000;
    sched.onNewRotation();

    sched.processNotification(); // should start at slice 3
    TEST_ASSERT_EQUAL_UINT8(3 * 30, g_sends[0].pixels[0].red);
}

// --- Runner ---

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_timer_slice_sends_current_slice);
    RUN_TEST(test_timer_slice_advances);
    RUN_TEST(test_timer_slice_stops_at_numslices);
    RUN_TEST(test_timer_slice_mirror);
    RUN_TEST(test_direct_push_sends_first_slice);
    RUN_TEST(test_direct_push_cycles_through_slices);
    RUN_TEST(test_direct_push_wraps_around);
    RUN_TEST(test_direct_push_clears_flag);
    RUN_TEST(test_timer_slice_takes_priority_over_direct_push);
    RUN_TEST(test_no_notification_returns_false);
    RUN_TEST(test_phase_offset_shifts_start_slice);
    return UNITY_END();
}
