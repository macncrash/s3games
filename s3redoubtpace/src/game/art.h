// S3 REDOUBT PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace redoubtpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_EARTH = 4,
    PAL_FIGURE = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_FLAG = 8
};

struct Art {
    gs::Mipped raider[2];
    gs::Mipped fallen;
    gs::Mipped gabion;
    gs::Mipped parapet;
    gs::Mipped stake;
    gs::Mipped flag;
    gs::Mipped bead;
    gs::Mipped flash;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace redoubtpace
