// S3 RAILLANE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lane {

enum Pal : int {
    PAL_TEXT = 0,
    PAL_DIM = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_AMBER = 4,
    PAL_CAB = 5,
    PAL_GATE = 6,
    PAL_POLE = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Image cab;
    gs::Image gate;
    gs::Image pole;
    gs::Image lamp;
    gs::Image logo;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lane
