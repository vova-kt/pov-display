#include "framebuffer.h"
#include <esp_heap_caps.h>
#include <cstring>

size_t Framebuffer::bufferSize() const {
    return (size_t)numSlices_ * numLeds_ * sizeof(Pixel);
}

bool Framebuffer::init(uint16_t numSlices, uint16_t numLeds) {
    numSlices_ = numSlices;
    numLeds_   = numLeds;
    front_     = 0;

    size_t sz = bufferSize();
    for (int i = 0; i < 2; i++) {
        buffers_[i] = (Pixel*)heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!buffers_[i]) {
            release();
            return false;
        }
        memset(buffers_[i], 0, sz);
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
    release();
    return init(numSlices, numLeds);
}

void Framebuffer::setPixel(uint16_t slice, uint16_t led,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint8_t back = 1 - front_;
    Pixel& p = buffers_[back][(size_t)slice * numLeds_ + led];
    p.brightness = 0xE0 | (brightness & 0x1F);
    p.blue  = b;
    p.green = g;
    p.red   = r;
}

void Framebuffer::clearBack() {
    uint8_t back = 1 - front_;
    size_t sz = (size_t)numSlices_ * numLeds_ * sizeof(Pixel);
    memset(buffers_[back], 0, sz);
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
