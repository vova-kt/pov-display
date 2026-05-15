#pragma once
#include <cstdint>
#include <cstring>

enum class ParamType : uint8_t {
    Int,    // numeric, honors min/max
    Bool,   // 0 / 1
    Color,  // 0xRRGGBB packed into value
    Text,   // string in textBuf (sized by textBufSize)
    Enum,   // pick from options[]
};

struct ParamOption {
    const char* label;
    int32_t     value;
};

struct Param {
    const char* key;
    const char* label;
    ParamType   type;
    int32_t     value;          // Color = 0xRRGGBB; Bool = 0/1; Enum/Int = native
    int32_t     defaultVal;
    int32_t     min;            // Int only; ignored for other types
    int32_t     max;            // Int only; ignored for other types
    const ParamOption* options; // Enum only
    uint8_t     optionCount;
    char*       textBuf;        // Text only — non-null and sized textBufSize
    uint16_t    textBufSize;
};

inline bool param_enum_value_allowed(const Param& p, int32_t v) {
    for (uint8_t i = 0; i < p.optionCount; i++)
        if (p.options[i].value == v) return true;
    return false;
}

inline void param_set_int(Param& p, int32_t v) {
    switch (p.type) {
        case ParamType::Int:
            if (v < p.min) v = p.min;
            if (v > p.max) v = p.max;
            p.value = v;
            break;
        case ParamType::Bool:
            p.value = v ? 1 : 0;
            break;
        case ParamType::Color:
            p.value = v & 0xFFFFFF;
            break;
        case ParamType::Enum:
            if (param_enum_value_allowed(p, v)) p.value = v;
            break;
        case ParamType::Text:
            break;
    }
}

inline void param_set_text(Param& p, const char* s) {
    if (p.type != ParamType::Text || !p.textBuf || p.textBufSize == 0) return;
    size_t n = strlen(s);
    if (n >= p.textBufSize) n = p.textBufSize - 1;
    memcpy(p.textBuf, s, n);
    p.textBuf[n] = '\0';
}
