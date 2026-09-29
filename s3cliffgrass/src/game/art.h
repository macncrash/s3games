// S3 CLIFF GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffgrass {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_TUFT = 2,
    PAL_ROCK = 3,
    PAL_FLAG = 4,
    PAL_ROCKROAD = 12,
    PAL_GRASS = 13
};

struct Art {
    gs::Mipped car;
    gs::Mipped tuft;
    gs::Mipped rock;
    gs::Mipped flag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffgrass
