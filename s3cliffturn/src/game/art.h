#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliff {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_GHOST = 2,
    PAL_ROCK = 3,
    PAL_SIGN = 4,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car[3];
    gs::Mipped ghost;
    gs::Mipped post;
    gs::Mipped board;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliff
