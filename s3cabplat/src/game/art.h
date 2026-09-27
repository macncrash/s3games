// S3 CAB PLAT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_PLAT = 5,
    PAL_RIVAL = 6,
    PAL_TOWN = 7,
    PAL_SIGN = 8
};

struct Art {
    gs::Mipped cab, wheel, plat, rival, lamp, sign, stripe;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabplat
