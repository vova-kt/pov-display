#include "config.h"

// NVS load/save for top-level Config fields is now owned by settings_registry.
// These methods remain as no-ops so existing callers (if any) compile cleanly.
void Config::loadFromNvs() {}
void Config::saveToNvs() {}
