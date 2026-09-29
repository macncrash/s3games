// S3 ORCHARDWELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchardwell {

enum Pal {
    PAL_HUD = 0,
    PAL_LEAF = 1,
    PAL_KEEPER = 2,
    PAL_STONE = 3,
    PAL_BOAR = 4,
    PAL_WOOD = 5,
    PAL_APPLE = 6,
    PAL_FX = 7
};

struct Art {
    gs::Mipped keeper, shove;
    gs::Mipped well, wellCrack, wellFell;
    gs::Mipped tree, apple;
    gs::Mipped boar, crate, ram;
    gs::Mipped dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchardwell
