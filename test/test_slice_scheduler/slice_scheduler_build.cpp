// Pull the real scheduler implementation into this test's compilation unit.
// slice_scheduler.cpp is not in the native build_src_filter because it
// depends on LedDriver, which only the scheduler test stubs.
#include "../../src/slice_scheduler.cpp"
