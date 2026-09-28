// S3 MILL BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_EARTH = 2,
    PAL_BANNER = 3,
    PAL_PLAYER = 4,
    PAL_HAND = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8
};

struct Art {
    gs::Mipped stand, runA, runB, swing;
    gs::Mipped hand[3];
    gs::Mipped banner[2];
    gs::Mipped pole;
    gs::Mipped mill;
    gs::Mipped wheel[2];
    gs::Mipped sack;
    gs::Mipped reed;
    gs::Mipped race;
    gs::Mipped sun;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
