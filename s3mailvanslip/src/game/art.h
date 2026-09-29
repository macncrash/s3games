// S3 MAILVAN SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailvanslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_QUAY = 2,
    PAL_WATER = 3,
    PAL_PIER = 4,
    PAL_POST = 5,
    PAL_WIN = 6,
    PAL_ALERT = 7,
    PAL_GOLD = 8,
    PAL_SACK = 9,
};

struct Art {
    gs::Mipped van;
    gs::Mipped pier;
    gs::Mipped water;
    gs::Mipped quay;
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped sack;
    gs::Mipped buoy;
    gs::Mipped title;
    gs::Mipped berthed;
    gs::Mipped missed;
    gs::Mipped tide;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailvanslip
