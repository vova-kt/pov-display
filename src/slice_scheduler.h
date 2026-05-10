#pragma once
#include <cstdint>
#include <Arduino.h>
#include <esp_timer.h>
#include "framebuffer.h"
#include "hal_spi_leds.h"
#include "hall_sensor.h"

class SliceScheduler {
public:
    void init(Framebuffer* fb, LedDriver* leds, HallSensor* hall);
    void start();
    void stop();
    void setPhaseOffset(int16_t offset) { phaseOffset_ = offset; }
    void setNumSlices(uint16_t n)       { numSlices_ = n; }

    static void renderTaskFunc(void* param);
    void onNewRotation();

private:
    static void IRAM_ATTR timerCallback(void* arg);

    Framebuffer*       fb_          = nullptr;
    LedDriver*         leds_        = nullptr;
    HallSensor*        hall_        = nullptr;
    esp_timer_handle_t timer_       = nullptr;
    TaskHandle_t       renderTask_  = nullptr;

    volatile uint16_t currentSlice_ = 0;
    volatile uint16_t numSlices_    = 360;
    volatile int16_t  phaseOffset_  = 0;
    volatile bool     running_      = false;
};
