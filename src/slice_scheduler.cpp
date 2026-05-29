#include "slice_scheduler.h"

void SliceScheduler::init(Framebuffer* fb, LedDriver* leds, TimingSource* timing) {
    fb_     = fb;
    leds_   = leds;
    timing_ = timing;

    esp_timer_create_args_t args = {};
    args.callback = timerCallback;
    args.arg      = this;
    args.name     = "slice";
    esp_err_t err = esp_timer_create(&args, &timer_);
    Serial.printf("[scheduler] timer create: %s\n", err == ESP_OK ? "OK" : "FAILED");

    BaseType_t ok = xTaskCreate(renderTaskFunc, "render", 4096, this, 24, &renderTask_);
    Serial.printf("[scheduler] render task create: %s (pri=24)\n", ok == pdPASS ? "OK" : "FAILED");
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

    uint32_t periodUs = timing_->rotationPeriodUs();
    if (periodUs == 0) return;

    uint32_t sliceIntervalUs = periodUs / numSlices_;
    if (sliceIntervalUs < 10) return;

    currentSlice_ = (uint16_t)((int16_t)0 + phaseOffset_);
    if ((int16_t)currentSlice_ < 0)
        currentSlice_ += numSlices_;
    currentSlice_ %= numSlices_;

    static uint32_t rotCount = 0;
    if (rotCount % 100 == 0) {
        Serial.printf("[scheduler] rot #%lu period=%luus sliceInt=%luus slices=%u\n",
                      rotCount, periodUs, sliceIntervalUs, numSlices_);
    }
    rotCount++;

    esp_timer_stop(timer_);
    esp_timer_start_periodic(timer_, sliceIntervalUs);
}

void SliceScheduler::requestDirectPush() {
    directPush_ = true;
    xTaskNotifyGive(renderTask_);
}

void IRAM_ATTR SliceScheduler::timerCallback(void* arg) {
    auto* self = static_cast<SliceScheduler*>(arg);

    if (self->currentSlice_ >= self->numSlices_) {
        esp_timer_stop(self->timer_);
        return;
    }

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(self->renderTask_, &woken);
    portYIELD_FROM_ISR(woken);
}

bool SliceScheduler::processNotification() {
    if (currentSlice_ < numSlices_) {
        uint16_t slice = currentSlice_;
        currentSlice_++;

        if (slice < fb_->numSlices()) {
            uint16_t fbSlice = mirror_
                ? (numSlices_ - 1 - slice) : slice;
            const Pixel* data = fb_->getSlice(fbSlice);
            leds_->sendSlice(data, fb_->numLeds());
        }
        return true;
    }
    if (directPush_) {
        directPush_ = false;
        uint16_t slice = directSlice_;
        directSlice_ = (directSlice_ + 1) % fb_->numSlices();
        const Pixel* data = fb_->getSlice(slice);
        leds_->sendSlice(data, fb_->numLeds());
        return true;
    }
    return false;
}

void SliceScheduler::renderTaskFunc(void* param) {
    auto* self = static_cast<SliceScheduler*>(param);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        self->processNotification();
    }
}
