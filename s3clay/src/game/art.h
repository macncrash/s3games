// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace clay {

enum Pal {
    PAL_HUD = 0,
    PAL_CLAY = 1,
    PAL_GUN = 2,
    PAL_HOUSE = 3,
    PAL_TREE = 4,
    PAL_SKY = 5,
    PAL_SHARD = 6,
    PAL_BEAD = 7,
    PAL_GOLD = 8,
    PAL_FLASH = 9,
    PAL_MATCH = 11,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped clay[6];
    gs::Mipped shard[4];
    gs::Mipped gun, house, tree[2], cloud, sun, hill, bead, flash, shadow, tuft;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clay
