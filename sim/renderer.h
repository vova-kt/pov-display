#pragma once
#include <cstdint>

class Framebuffer;
struct Config;

bool  renderer_init();
void  renderer_resize(int width, int height);
void  renderer_render(const Framebuffer& fb, const Config& cfg,
                      float armAngle, float armSweep,
                      float hallOffsetAngle, bool hasOverruns,
                      int numSlices);
void  renderer_set_hub_fraction(float f);
void  renderer_set_gap_fraction(float f);
void  renderer_set_show_overruns(bool v);
void  renderer_set_show_hall_marker(bool v);
void  renderer_set_show_slice_grid(bool v);
void  renderer_set_num_arms(int n);
