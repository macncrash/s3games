// S3 CRANE GRASS pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranegrass {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_YARD = 4,
    PAL_CRANE = 5,
    PAL_BOOM = 6,
    PAL_FX = 7
};

constexpr int HEADINGS = 16;

struct Art {
    gs::Mipped grass, slab, water, tree, puff, shadow, legs;
    gs::Mipped crane[HEADINGS];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranegrass
