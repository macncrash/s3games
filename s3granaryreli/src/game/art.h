// Granary sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace granary {

enum Pal {
    PAL_HUD = 0,
    PAL_BARN = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_CART = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped barn, silo, sack;
    gs::Mipped fork, sight, puff;
    gs::Mipped thief[2], runner[2], wagon[2];
    gs::Mipped keeper;
    gs::Mipped bell, rope;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granary
