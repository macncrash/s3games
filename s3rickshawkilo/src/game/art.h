// Drawn at boot into sprite ROM and tile RAM. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_RIVAL = 5,
    PAL_WHEEL = 6,
    PAL_CART = 7,
    PAL_STALL = 8,
    PAL_TREE = 9,
    PAL_SKY = 10,
    PAL_ROAD = 11,
};

struct Art {
    gs::Mipped cab;
    gs::Mipped rival;
    gs::Mipped wheel;
    gs::Mipped cart;
    gs::Mipped stall;
    gs::Mipped tree;
    gs::Mipped puller;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilo
