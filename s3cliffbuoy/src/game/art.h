// S3 CLIFFBUOY sprites and the cove picture. Everything is drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffbuoy {

enum Pal {
    PAL_WHITE = 0,
    PAL_COVE = 1,
    PAL_SKIFF = 2,
    PAL_BUOY = 3,
    PAL_WAKE = 4,
    PAL_MARK = 5,
    PAL_AMBER = 6,
    PAL_RED = 7,
    PAL_GREEN = 8,
    PAL_BANNER = 9
};

struct Art {
    gs::Mipped skiff[8];
    gs::Mipped buoy;
    gs::Mipped buoyDone;
    gs::Mipped wake;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffbuoy
