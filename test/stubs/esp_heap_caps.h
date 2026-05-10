#pragma once
#include <cstdlib>
#include <cstdint>

#define MALLOC_CAP_DMA   0
#define MALLOC_CAP_8BIT  0

inline void* heap_caps_malloc(size_t size, uint32_t) {
    return malloc(size);
}

inline void heap_caps_free(void* ptr) {
    free(ptr);
}
