// S3 CRANE KILO sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranekilo {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CRANE = 4,
    PAL_WHEEL = 5,
    PAL_YARD = 6,
    PAL_HOOK = 7,
    PAL_GATE = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped boom, hook, dash, wheel, axle, shed, lamp, post, banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranekilo
