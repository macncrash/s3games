// S3 SCULLLOCK pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scull {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_HULL = 2,
    PAL_WOOD = 3,
    PAL_STONE = 4,
    PAL_BUOY = 5,
    PAL_OAR = 6,
    PAL_BANK = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped hull;
    gs::Mipped oar[5];
    gs::Mipped leaf;
    gs::Mipped post;
    gs::Mipped pier;
    gs::Mipped tree;
    gs::Mipped reed;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scull
