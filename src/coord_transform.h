#pragma once

class Canvas;
class Framebuffer;
struct Config;

class CoordTransform {
public:
    virtual ~CoordTransform() = default;
    virtual void apply(const Canvas& canvas, Framebuffer& fb, const Config& cfg) = 0;
};
