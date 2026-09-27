// S3 BARGE KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bkilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_BOAT = 4,
    PAL_RIVAL = 5,
    PAL_MILL = 6,
    PAL_CRANE = 7,
    PAL_CART = 8,
    PAL_WATER = 9,
    PAL_BANK = 10,
    PAL_TREE = 11,
    PAL_SKY = 12,
    PAL_BANNER = 13,
    PAL_POST = 14,
};

struct Art {
    gs::Mipped barge;
    gs::Mipped rival;
    gs::Mipped wheel[4];
    gs::Mipped mill;
    gs::Mipped crane;
    gs::Mipped cart;
    gs::Mipped reed;
    gs::Mipped tree;
    gs::Mipped house;
    gs::Mipped water;
    gs::Mipped bank;
    gs::Mipped banner;
    gs::Mipped gull[2];
    gs::Mipped cloud;
    gs::Mipped sun;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bkilo
