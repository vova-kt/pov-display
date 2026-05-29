#pragma once
#include <esp_log.h>

// Per-file usage:
//   #include "log_tags.h"
//   LOG_TAG(motor);
//   POV_LOGI("init on GPIO%u", pin);
//
// Tag exclusion: add to platformio.ini build_flags:
//   -DLOG_EXCLUDE_TAGS='"scanner","spi"'
// Then call pov_log_apply_exclusions() in setup() after Serial.begin().

#define LOG_TAG(tag) static const char* TAG __attribute__((unused)) = #tag

#define POV_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define POV_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#define POV_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define POV_LOGD(fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define POV_LOGV(fmt, ...) ESP_LOGV(TAG, fmt, ##__VA_ARGS__)

inline void pov_log_apply_exclusions() {
#ifdef LOG_EXCLUDE_TAGS
    static const char* excluded[] = { LOG_EXCLUDE_TAGS };
    for (const char* tag : excluded)
        esp_log_level_set(tag, ESP_LOG_NONE);
#endif
}
