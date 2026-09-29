// S3 CHEFCHIME pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace chefchime {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_OK = 3,
    PAL_CHEF = 4,
    PAL_FOOD = 5,
    PAL_STEEL = 6,
    PAL_FIRE = 7,
    PAL_CLOCK = 8,
    PAL_WOOD = 9
};

struct Art {
    int font[96] = {};
    gs::Image chef = {};
    gs::Image chefWalk = {};
    gs::Image roast = {};
    gs::Image pan = {};
    gs::Image flame[2] = {};
    gs::Image clock = {};
    gs::Image pendulum = {};
    gs::Image door = {};
    gs::Image solid = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace chefchime
