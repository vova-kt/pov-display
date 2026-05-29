#pragma once
#include <cstdint>

typedef int BaseType_t;
typedef uint32_t TickType_t;

#define pdFALSE 0
#define pdTRUE  1
#define pdPASS  1
#define portMAX_DELAY 0xFFFFFFFF
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
