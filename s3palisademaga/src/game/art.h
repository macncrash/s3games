#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisade {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_EARTH = 2,
    PAL_PLAYER = 3,
    PAL_RAIDER = 4,
    PAL_FEINT = 5,
    PAL_FX = 6,
    PAL_GRASS = 8
};

struct Art {
    gs::Image sentry;
    gs::Image raider;
    gs::Image feint;
    gs::Image stake;
    gs::Image round;
    gs::Image slug;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisade
