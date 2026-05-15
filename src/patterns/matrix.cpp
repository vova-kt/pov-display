#include "matrix.h"

static constexpr uint8_t kGlyphW = 5;
static constexpr uint8_t kGlyphH = 7;
static constexpr uint8_t kColumnStride = 7;
static constexpr uint8_t kCellPitch = 8;
static constexpr uint8_t kMaxTrailCells = 7;
static constexpr uint8_t kStreamsPerColumn = 2;
static constexpr int32_t kDefaultSpeedPxPerSec = 12;
static constexpr int32_t kMaxSpeedPxPerSec = 48;

enum MatrixGlyphIndex : uint8_t {
    GLYPH_CYR_EF = 44,
    GLYPH_CYR_U,
    GLYPH_CYR_ZE,
    GLYPH_CYR_I,
    GLYPH_CYR_O,
    GLYPH_CYR_EN,
};

// 5x7 row-major glyphs: ASCII, then Greek, Cyrillic, and a few compact CJK
// ideographs. Selection is hash-based; table order is deliberately not used
// as an animation sequence.
static const uint8_t kGlyphs[][kGlyphH] = {
    {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}, // 0
    {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // 1
    {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}, // 2
    {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}, // 3
    {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}, // 4
    {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110}, // 5
    {0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}, // 6
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}, // 7
    {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}, // 8
    {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110}, // 9
    {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, // A
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}, // B
    {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111}, // C
    {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110}, // D
    {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, // E
    {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}, // F
    {0b01111, 0b10000, 0b10000, 0b10011, 0b10001, 0b10001, 0b01111}, // G
    {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, // H
    {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // I
    {0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100}, // J
    {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}, // K
    {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, // L
    {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}, // M
    {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001}, // N
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // O
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, // P
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}, // Q
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}, // R
    {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}, // S
    {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}, // T
    {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // U
    {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}, // V
    {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010}, // W
    {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}, // X
    {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}, // Y
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}, // Z
    {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, // Greek alpha
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}, // Greek beta
    {0b00100, 0b01010, 0b01010, 0b10001, 0b10001, 0b11111, 0b10001}, // Greek lambda
    {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001}, // Greek omega
    {0b11111, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010}, // Greek pi
    {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, // Greek sigma
    {0b00100, 0b01110, 0b10101, 0b10101, 0b01110, 0b00100, 0b00100}, // Greek phi
    {0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001, 0b10001}, // Greek chi
    {0b01110, 0b10101, 0b10101, 0b10101, 0b01110, 0b00100, 0b00100}, // Cyrillic Ef
    {0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110}, // Cyrillic u
    {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}, // Cyrillic Ze
    {0b10001, 0b10011, 0b10101, 0b10101, 0b11001, 0b10001, 0b10001}, // Cyrillic I
    {0b00000, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}, // Cyrillic o
    {0b00000, 0b00000, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001}, // Cyrillic en
    {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}, // Cyrillic Ge
    {0b10001, 0b10101, 0b01110, 0b00100, 0b01110, 0b10101, 0b10001}, // Cyrillic Zhe
    {0b11111, 0b00100, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100}, // CJK-ish center
    {0b11111, 0b10001, 0b11111, 0b10001, 0b10001, 0b11111, 0b00000}, // CJK-ish sun
    {0b11111, 0b00100, 0b11111, 0b10101, 0b00100, 0b10101, 0b00000}, // CJK-ish rain
};
static constexpr uint8_t kGlyphCount = sizeof(kGlyphs) / sizeof(kGlyphs[0]);
static const uint8_t kTrailStrengths[kMaxTrailCells] = {255, 190, 135, 90, 55, 32, 20};
// Rare vertical easter egg: ФуЗИон, reversed here because trail cells are
// stored from bright head upward while the viewer reads the column top-down.
static const uint8_t kFusionGlyphs[] = {
    GLYPH_CYR_EF, GLYPH_CYR_U, GLYPH_CYR_ZE,
    GLYPH_CYR_I, GLYPH_CYR_O, GLYPH_CYR_EN,
};

MatrixPattern::MatrixPattern() {
    storage_[0] = {
        "speed", "Speed px/s", ParamType::Int,
        kDefaultSpeedPxPerSec, kDefaultSpeedPxPerSec, 1, kMaxSpeedPxPerSec,
        nullptr, 0,
        nullptr, 0
    };
    params_ = storage_;
    paramCount_ = 1;
}

void MatrixPattern::ensureCanvas(uint16_t numSlices, uint16_t numLeds) {
    uint16_t cw = numLeds * 2;
    uint16_t ch = numLeds * 2;

    if (cw == lastCanvasW_ && ch == lastCanvasH_ &&
        numSlices == lastSlices_ && numLeds == lastLeds_)
        return;

    canvas_.release();
    canvas_.init(cw, ch);
    transform_.buildLut(numSlices, numLeds, cw, ch);
    lastCanvasW_ = cw;
    lastCanvasH_ = ch;
    lastSlices_ = numSlices;
    lastLeds_ = numLeds;
}

static uint16_t hash16(uint16_t x) {
    x ^= x >> 7;
    x *= 0x2C1B;
    x ^= x >> 9;
    x *= 0x11B5;
    x ^= x >> 8;
    return x;
}

static uint8_t glyphAt(uint16_t streamSeed, uint16_t row) {
    return (uint8_t)(hash16((uint16_t)(streamSeed ^ (row * 0x63))) % kGlyphCount);
}

static void setIfBrighter(Canvas& canvas, int16_t x, int16_t y,
                          uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    if (x < 0 || y < 0 || x >= (int16_t)canvas.width() || y >= (int16_t)canvas.height()) return;
    if (brightness == 0 || (r == 0 && g == 0 && b == 0)) return;

    Pixel p = canvas.pixelAt((uint16_t)x, (uint16_t)y);
    uint16_t oldScore = p.brightness == 0 ? 0 : (uint16_t)(p.brightness & 0x1F) * p.green;
    uint16_t newScore = (uint16_t)brightness * g;
    if (oldScore > newScore) return;

    canvas.setPixel((uint16_t)x, (uint16_t)y, r, g, b, brightness);
}

static uint8_t scaledBrightness(const Config& cfg, uint8_t strength) {
    return (uint8_t)(((uint16_t)cfg.brightness * strength + 127) / 255);
}

static void drawGreenPixel(Canvas& canvas, int16_t x, int16_t y,
                           const Config& cfg, uint8_t strength, bool head) {
    uint8_t brightness = scaledBrightness(cfg, strength);
    if (brightness == 0) return;

    uint8_t green = strength;
    uint8_t red = head ? (uint8_t)(((uint16_t)strength * 170) / 255) : 0;
    uint8_t blue = head ? (uint8_t)(((uint16_t)strength * 120) / 255) : 0;
    setIfBrighter(canvas, x, y, red, green, blue, brightness);
}

static void drawGlyph(Canvas& canvas, int16_t startX, int16_t topY,
                      uint8_t glyphIndex, const Config& cfg, uint8_t strength, bool head) {
    const uint8_t* glyph = kGlyphs[glyphIndex % kGlyphCount];

    for (uint8_t y = 0; y < kGlyphH; y++) {
        uint8_t row = glyph[y];
        for (uint8_t x = 0; x < kGlyphW; x++) {
            if ((row & (1 << (kGlyphW - 1 - x))) == 0) continue;

            int16_t px = startX + x;
            int16_t py = topY + y;
            uint8_t glow = strength / 5;
            drawGreenPixel(canvas, px - 1, py, cfg, glow, false);
            drawGreenPixel(canvas, px + 1, py, cfg, glow, false);
            drawGreenPixel(canvas, px, py - 1, cfg, glow, false);
            drawGreenPixel(canvas, px, py + 1, cfg, glow, false);
            drawGreenPixel(canvas, px, py, cfg, strength, head);
        }
    }
}

void MatrixPattern::generate(Framebuffer& fb, const Config& cfg, uint32_t timeMs) {
    fb.clearBack();

    uint16_t numSlices = fb.numSlices();
    uint16_t numLeds = fb.numLeds();
    if (numSlices == 0 || numLeds == 0) return;

    ensureCanvas(numSlices, numLeds);
    canvas_.clear();

    int32_t speed = storage_[0].value;
    if (speed < 1) speed = 1;
    if (speed > kMaxSpeedPxPerSec) speed = kMaxSpeedPxPerSec;

    uint32_t phasePx = (timeMs / 1000) * (uint32_t)speed +
                       ((timeMs % 1000) * (uint32_t)speed) / 1000;

    uint16_t cw = canvas_.width();
    uint16_t ch = canvas_.height();
    uint16_t numCols = (cw + kColumnStride - 1) / kColumnStride;
    uint16_t numRows = ch / kCellPitch;
    if (numRows == 0) return;

    for (uint16_t col = 0; col < numCols; col++) {
        uint16_t colSeed = hash16((uint16_t)(col + 1));
        int16_t colX = (int16_t)(col * kColumnStride + (colSeed & 1));

        for (uint8_t stream = 0; stream < kStreamsPerColumn; stream++) {
            uint16_t seed = hash16((uint16_t)(colSeed ^ (stream * 0x6D) ^ 0x9E37));
            bool fusionStream = stream == 1 && (col % 13) == 3;
            uint8_t trail = 5 + (seed % 3);
            if (fusionStream && trail < sizeof(kFusionGlyphs))
                trail = sizeof(kFusionGlyphs);
            uint8_t gap = 3 + ((seed >> 5) % 8);
            uint16_t cycleLen = numRows + trail + gap;
            uint32_t cyclePx = (uint32_t)cycleLen * kCellPitch;
            uint32_t offsetPx = (uint32_t)(seed % cycleLen) * kCellPitch;
            uint32_t headPx = (phasePx + offsetPx) % cyclePx;
            uint32_t trailPx = (uint32_t)trail * kCellPitch;

            for (uint16_t row = 0; row < numRows; row++) {
                uint32_t rowPx = (uint32_t)row * kCellPitch;
                uint32_t pastPx = (headPx >= rowPx)
                    ? headPx - rowPx
                    : headPx + cyclePx - rowPx;

                if (pastPx >= trailPx) continue;

                uint8_t cellIdx = (uint8_t)(pastPx / kCellPitch);
                uint8_t frac = (uint8_t)(pastPx % kCellPitch);

                uint8_t s0 = kTrailStrengths[cellIdx];
                uint8_t s1 = (cellIdx + 1 < kMaxTrailCells)
                    ? kTrailStrengths[cellIdx + 1] : 0;
                uint8_t strength = (uint8_t)(s0 -
                    ((uint16_t)(s0 - s1) * frac) / kCellPitch);

                bool isHead = (cellIdx == 0);

                uint8_t glyph;
                if (fusionStream && cellIdx < sizeof(kFusionGlyphs) &&
                    ((phasePx / kCellPitch + (seed & 0x0F)) % 53) < 21) {
                    glyph = kFusionGlyphs[sizeof(kFusionGlyphs) - 1 - cellIdx];
                } else {
                    glyph = glyphAt(seed, row);
                }

                drawGlyph(canvas_, colX, (int16_t)(row * kCellPitch),
                          glyph, cfg, strength, isHead);
            }
        }
    }

    transform_.apply(canvas_, fb, cfg);
}
