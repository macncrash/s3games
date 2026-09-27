// S3 CAB KILO sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_WHEEL = 5,
    PAL_BODY = 6,
    PAL_STONE = 7,
    PAL_SIGN = 8,
    PAL_POST = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped dash, helm[3], wheel, body, block, lamp, post, banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabkilo
