#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisade {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_STONE = 2,
    PAL_PLAYER = 3,
    PAL_RAIDER = 4,
    PAL_BRUTE = 5,
    PAL_FX = 6,
    PAL_GRASS = 8
};

struct Art {
    gs::Image player;
    gs::Image raider;
    gs::Image brute;
    gs::Image well;
    gs::Image post;
    gs::Image arrow;
    gs::Image moon;
    gs::Image pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisade
