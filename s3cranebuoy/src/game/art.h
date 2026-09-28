// S3 CRANEBUOY sprites and the basin picture. Everything is drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranebuoy {

enum Pal {
    PAL_WHITE = 0,
    PAL_HARBOR = 1,
    PAL_CRANE = 2,
    PAL_BUOY = 3,
    PAL_WAKE = 4,
    PAL_MARK = 5,
    PAL_AMBER = 6,
    PAL_RED = 7,
    PAL_GREEN = 8,
    PAL_BANNER = 9
};

struct Art {
    gs::Mipped crane[8];
    gs::Mipped buoy;
    gs::Mipped buoyDone;
    gs::Mipped wake;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranebuoy
