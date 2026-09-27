// S3 ALLEYWELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alleywell {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_KEEPER = 2,
    PAL_WOOD = 3,
    PAL_IRON = 4,
    PAL_BRICK = 5,
    PAL_FX = 6,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped well, wellCrack, wellFell;
    gs::Mipped keeper, shove;
    gs::Mipped barrel, cart, ram;
    gs::Mipped pier, window;
    gs::Mipped dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alleywell
