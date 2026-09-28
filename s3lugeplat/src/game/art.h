// S3 LUGE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_RIDER = 2,
    PAL_WOOD = 3,
    PAL_ICE = 4,
    PAL_TREE = 5,
    PAL_HILL = 6,
    PAL_MARK = 7
};

struct Art {
    gs::Mipped sled;
    gs::Mipped plat;
    gs::Mipped post;
    gs::Mipped tree;
    gs::Mipped hill;
    gs::Mipped ice;
    gs::Mipped chip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
