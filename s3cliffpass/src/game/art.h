// S3 CLIFF PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffpass {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_CREW = 2,
    PAL_ROCK = 3,
    PAL_GATE = 4,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car;
    gs::Mipped crew;
    gs::Mipped rock;
    gs::Mipped gate;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffpass
