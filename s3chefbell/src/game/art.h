// S3 CHEFBELL pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace chefbell {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_OK = 3,
    PAL_CHEF = 4,
    PAL_FOOD = 5,
    PAL_STEEL = 6,
    PAL_FIRE = 7,
    PAL_BELL = 8,
    PAL_BAR = 9
};

struct Art {
    int font[96] = {};
    gs::Image chef = {};
    gs::Image chefReach = {};
    gs::Image steak = {};
    gs::Image pan = {};
    gs::Image flame[2] = {};
    gs::Image bell = {};
    gs::Image clapper = {};
    gs::Image solid = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace chefbell
