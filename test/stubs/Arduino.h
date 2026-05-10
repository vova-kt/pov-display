#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>

struct SerialStub {
    void printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
};

static SerialStub Serial;
