#include "framebuffer.h"
#include <esp_heap_caps.h>
#include <cstring>

// Seed one pixel, then exponentially double with memcpy.
// ~log2(count) memcpy calls instead of count scalar stores.
static void fillBlack(Pixel* buf, size_t count) {
    if (count == 0) return;
    buf[0] = kBlackPixel;
    size_t filled = 1;
    while (filled < count) {
        size_t chunk = count - filled;
        if (chunk > filled) chunk = filled;
        memcpy(buf + filled, buf, chunk * sizeof(Pixel));
        filled += chunk;
    }
}

size_t Framebuffer::bufferSize() const {
    return (size_t)numSlices_ * numLeds_ * sizeof(Pixel);
}

bool Framebuffer::init(uint16_t numSlices, uint16_t numLeds) {
    numSlices_ = numSlices;
    numLeds_   = numLeds;
    front_     = 0;

    size_t sz = bufferSize();
    size_t count = (size_t)numSlices * numLeds;
    for (int i = 0; i < 2; i++) {
        buffers_[i] = (Pixel*)heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!buffers_[i]) {
            release();
            return false;
        }
        fillBlack(buffers_[i], count);
    }
    return true;
}

void Framebuffer::release() {
    for (int i = 0; i < 2; i++) {
        if (buffers_[i]) {
            heap_caps_free(buffers_[i]);
            buffers_[i] = nullptr;
        }
    }
}

bool Framebuffer::resize(uint16_t numSlices, uint16_t numLeds) {
    size_t sz = (size_t)numSlices * numLeds * sizeof(Pixel);

    size_t count = (size_t)numSlices * numLeds;
    Pixel* newBufs[2] = {nullptr, nullptr};
    for (int i = 0; i < 2; i++) {
        newBufs[i] = (Pixel*)heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!newBufs[i]) {
            for (int j = 0; j < i; j++) heap_caps_free(newBufs[j]);
            return false;
        }
        fillBlack(newBufs[i], count);
    }

    release();
    buffers_[0] = newBufs[0];
    buffers_[1] = newBufs[1];
    numSlices_ = numSlices;
    numLeds_   = numLeds;
    front_     = 0;
    return true;
}

void Framebuffer::setPixel(uint16_t slice, uint16_t led,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint8_t back = 1 - front_;
    Pixel& p = buffers_[back][(size_t)slice * numLeds_ + led];
    p.brightness = kHd107sBrightnessPrefix | (brightness & kHd107sBrightnessMask);
    p.blue  = b;
    p.green = g;
    p.red   = r;
}

void Framebuffer::clearBack() {
    uint8_t back = 1 - front_;
    fillBlack(buffers_[back], (size_t)numSlices_ * numLeds_);
}

const Pixel* Framebuffer::getSlice(uint16_t sliceIndex) const {
    return &buffers_[front_][(size_t)sliceIndex * numLeds_];
}

Pixel* Framebuffer::backSlice(uint16_t sliceIndex) {
    uint8_t back = 1 - front_;
    return &buffers_[back][(size_t)sliceIndex * numLeds_];
}

Pixel* Framebuffer::backBuffer() {
    return buffers_[1 - front_];
}

void Framebuffer::swap() {
    front_ = 1 - front_;
}
