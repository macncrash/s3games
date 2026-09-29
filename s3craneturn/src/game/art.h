// S3 CRANE TURN sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace craneturn {

enum Pal {
    PAL_HUD = 0,
    PAL_INK = 1,
    PAL_CRANE = 2,
    PAL_BOOM = 3,
    PAL_YARD = 4,
    PAL_CREW = 5,
    PAL_WARN = 6,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped body, boom, hook, wheel, pylon, crew, lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace craneturn
