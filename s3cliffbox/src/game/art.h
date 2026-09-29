// S3 CLIFF BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffbox {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_POST = 2,
    PAL_ROCK = 3,
    PAL_SIGN = 4,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car;
    gs::Mipped post;
    gs::Mipped rock;
    gs::Mipped sign;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffbox
