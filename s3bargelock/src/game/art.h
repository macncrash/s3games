// S3 BARGE LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargelock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_STONE = 2,
    PAL_GATE = 3,
    PAL_BANK = 4,
    PAL_HOUSE = 5,
    PAL_WATER = 6,
    PAL_FOAM = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
    PAL_POST = 11,
};

struct Art {
    gs::Mipped hull;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped grass;
    gs::Mipped house;
    gs::Mipped tree;
    gs::Mipped lamp;
    gs::Mipped foam;
    gs::Mipped plank;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargelock
