#include "registry.h"
#include <cstring>
#include "solid.h"
#include "rainbow.h"
#include "text.h"
#include "scanner.h"
#include "image.h"
#include "matrix.h"

static SolidPattern   s_solid;
static RainbowPattern s_rainbow;
static TextPattern    s_text;
static ScannerPattern s_scanner;
static ImagePattern   s_image;
static MatrixPattern  s_matrix;

// Order matters: activePattern indices reference this order.
Pattern* const g_patterns[] = {
    &s_solid,    // 0
    &s_rainbow,  // 1
    &s_text,     // 2
    &s_scanner,  // 3
    &s_image,    // 4
    &s_matrix,   // 5
};
const uint8_t G_NUM_PATTERNS = sizeof(g_patterns) / sizeof(g_patterns[0]);

int g_pattern_index(const char* key) {
    for (uint8_t i = 0; i < G_NUM_PATTERNS; i++)
        if (strcmp(g_patterns[i]->key(), key) == 0) return i;
    return -1;
}

ImagePattern* g_image_pattern() { return &s_image; }
