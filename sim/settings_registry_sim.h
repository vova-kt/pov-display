#pragma once
#include "timing.h"
#include "../src/config.h"
#include "../src/framebuffer.h"

void  sim_registry_bind(TimingState* ts, Config* cfg, Framebuffer* fb);
void  sim_apply_geometry(int numLeds);
float sim_settings_get_sim_speed();
