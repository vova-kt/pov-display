#pragma once
#include <cstdint>

typedef int esp_err_t;
#define ESP_OK 0

typedef void* esp_timer_handle_t;

typedef void (*esp_timer_cb_t)(void* arg);

struct esp_timer_create_args_t {
    esp_timer_cb_t callback;
    void* arg;
    const char* name;
};

inline esp_err_t esp_timer_create(const esp_timer_create_args_t*, esp_timer_handle_t*) { return ESP_OK; }
inline esp_err_t esp_timer_start_periodic(esp_timer_handle_t, uint64_t) { return ESP_OK; }
inline esp_err_t esp_timer_stop(esp_timer_handle_t) { return ESP_OK; }
