// S3 MAILVAN LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailvanlock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_ROAD = 2,
    PAL_GATE = 3,
    PAL_BANK = 4,
    PAL_OFFICE = 5,
    PAL_POST = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_GOLD = 9,
    PAL_MAIL = 10,
};

struct Art {
    gs::Mipped van;
    gs::Mipped gate;
    gs::Mipped road;
    gs::Mipped bank;
    gs::Mipped office;
    gs::Mipped box;
    gs::Mipped lamp;
    gs::Mipped tree;
    gs::Mipped sack;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailvanlock
