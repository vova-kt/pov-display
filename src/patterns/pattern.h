#pragma once
#include <cstdint>
#include <cstring>
#include "../framebuffer.h"
#include "../config.h"
#include "../param.h"

class Pattern {
public:
    virtual ~Pattern() = default;
    virtual void generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) = 0;
    virtual const char* name() const = 0;
    virtual const char* key() const { return name(); }

    uint8_t paramCount() const { return paramCount_; }
    Param& param(uint8_t i) { return params_[i]; }
    const Param& param(uint8_t i) const { return params_[i]; }

    Param* findParam(const char* paramKey) {
        for (uint8_t i = 0; i < paramCount_; i++)
            if (strcmp(params_[i].key, paramKey) == 0) return &params_[i];
        return nullptr;
    }

    void resetDefaults() {
        for (uint8_t i = 0; i < paramCount_; i++) {
            params_[i].value = params_[i].defaultVal;
            if (params_[i].type == ParamType::Text && params_[i].textBuf)
                params_[i].textBuf[0] = '\0';
        }
    }

protected:
    Param*  params_ = nullptr;
    uint8_t paramCount_ = 0;
};
