// S3 CRANE sprites and the dock picture. Everything is drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace crane {

enum Pal {
    PAL_WHITE = 0,
    PAL_YARD = 1,
    PAL_CRANE = 2,
    PAL_HOOK = 3,
    PAL_WOOD = 4,
    PAL_CABLE = 5,
    PAL_SPLASH = 6,
    PAL_BANNER = 7,
    PAL_AMBER = 8,
    PAL_RED = 9,
    PAL_GREEN = 10,
    PAL_SET = 11
};

struct Art {
    gs::Mipped trolley, hook, link, shadow, splash, arrow, bird, lamp, banner;
    gs::Mipped crate[3];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace crane
