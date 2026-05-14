#include "slice_scheduler.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void SliceScheduler::init(Framebuffer* fb, LedDriver* leds, HallSensor* hall) {
    fb_   = fb;
    leds_ = leds;
    hall_ = hall;

    esp_timer_create_args_t args = {};
    args.callback = timerCallback;
    args.arg      = this;
    args.name     = "slice";
    esp_timer_create(&args, &timer_);

    xTaskCreate(renderTaskFunc, "render", 4096, this, 24, &renderTask_);
}

void SliceScheduler::start() {
    running_ = true;
}

void SliceScheduler::stop() {
    running_ = false;
    esp_timer_stop(timer_);
}

void SliceScheduler::onNewRotation() {
    if (!running_) return;

    uint32_t periodUs = hall_->rotationPeriodUs();
    if (periodUs == 0) return;

    uint32_t sliceIntervalUs = periodUs / numSlices_;
    if (sliceIntervalUs < 10) return;  // sanity: minimum 10µs per slice

    currentSlice_ = (uint16_t)((int16_t)0 + phaseOffset_);
    if ((int16_t)currentSlice_ < 0)
        currentSlice_ += numSlices_;
    currentSlice_ %= numSlices_;

    esp_timer_stop(timer_);
    esp_timer_start_periodic(timer_, sliceIntervalUs);
}

void IRAM_ATTR SliceScheduler::timerCallback(void* arg) {
    auto* self = static_cast<SliceScheduler*>(arg);

    if (self->currentSlice_ >= self->numSlices_) {
        esp_timer_stop(self->timer_);
        return;
    }

    // Notify render task — the task reads currentSlice_ and sends SPI
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(self->renderTask_, &woken);
    portYIELD_FROM_ISR(woken);
}

void SliceScheduler::renderTaskFunc(void* param) {
    auto* self = static_cast<SliceScheduler*>(param);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint16_t slice = self->currentSlice_;
        self->currentSlice_++;

        if (slice < self->fb_->numSlices()) {
            const Pixel* data = self->fb_->getSlice(slice);
            self->leds_->sendSlice(data, self->fb_->numLeds());
        }
    }
}
