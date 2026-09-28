// S3 LUGE KILO sprites. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_HUD = 0,
    PAL_ICE = 1,
    PAL_LUGE = 2,
    PAL_STEEL = 3,
    PAL_CLOCK = 4,
    PAL_CART = 5,
    PAL_SHEAVE = 6,
    PAL_FX = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped luge[3];
    gs::Mipped wheel;
    gs::Mipped clock;
    gs::Mipped sheave;
    gs::Mipped ghost;
    gs::Mipped puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
