#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace boccebell {

enum Pal {
    PAL_COURT = 0,
    PAL_BOWL = 1,
    PAL_PALLINO = 2,
    PAL_BELL = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_DEAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image court;
    gs::Image bowl;
    gs::Image pallino;
    gs::Image bell;
    gs::Image banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boccebell
