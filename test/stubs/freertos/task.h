#pragma once
#include "FreeRTOS.h"

typedef void* TaskHandle_t;

inline BaseType_t xTaskCreate(void (*)(void*), const char*, uint32_t, void*, int, TaskHandle_t*) { return pdPASS; }
inline uint32_t ulTaskNotifyTake(BaseType_t, TickType_t) { return 0; }
inline void vTaskNotifyGiveFromISR(TaskHandle_t, BaseType_t*) {}
inline void xTaskNotifyGive(TaskHandle_t) {}
inline void vTaskDelay(TickType_t) {}

#define portYIELD_FROM_ISR(x) (void)(x)
