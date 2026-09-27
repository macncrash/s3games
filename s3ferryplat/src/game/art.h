// S3 FERRY PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferryplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_PIER = 5,
    PAL_WATER = 6,
    PAL_PILE = 7,
    PAL_HILL = 8,
    PAL_SKY = 9,
    PAL_END = 10,
    PAL_SHED = 11,
    PAL_FOAM = 12,
    PAL_GULL = 13,
    PAL_SIGN = 14,
    PAL_WAKE = 15
};

struct Art {
    gs::Mipped ferry;
    gs::Mipped plank, stripe, pile;
    gs::Mipped water, hill, cloud, sun;
    gs::Mipped shed, lamp, sign, gull[2], foam, wake;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferryplat
