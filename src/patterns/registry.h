#pragma once
#include <cstdint>
#include "pattern.h"

class ImagePattern;

extern Pattern* const g_patterns[];
extern const uint8_t G_NUM_PATTERNS;

int g_pattern_index(const char* key);
ImagePattern* g_image_pattern();

void loadPatternsFromNvs();
void savePatternsToNvs();
