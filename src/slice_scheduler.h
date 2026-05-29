#pragma once
#include <cstdint>
#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "framebuffer.h"
#include "hal_spi_leds.h"
#include "timing_source.h"

class SliceScheduler {
public:
    void init(Framebuffer* fb, LedDriver* leds, TimingSource* timing);
    void start();
    void stop();
    void setPhaseOffset(int16_t offset) { phaseOffset_ = offset; }
    void setNumSlices(uint16_t n)       { numSlices_ = n; }
    void setMirror(bool m)              { mirror_ = m; }

    void requestDirectPush();
    bool processNotification();
    static void renderTaskFunc(void* param);
    void onNewRotation();

private:
    static void IRAM_ATTR timerCallback(void* arg);

    Framebuffer*       fb_          = nullptr;
    LedDriver*         leds_        = nullptr;
    TimingSource*      timing_      = nullptr;
    esp_timer_handle_t timer_       = nullptr;
    TaskHandle_t       renderTask_  = nullptr;

    volatile uint16_t currentSlice_ = 0;
    volatile uint16_t numSlices_    = 360;
    volatile int16_t  phaseOffset_  = 0;
    volatile bool     mirror_       = true;
    volatile bool     running_      = false;
    volatile bool     directPush_   = false;
    uint16_t          directSlice_  = 0;
};
