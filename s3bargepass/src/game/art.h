// S3 BARGE PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargepass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_CLIFF = 2,
    PAL_ARCH = 3,
    PAL_PINE = 4,
    PAL_RIVAL = 5,
    PAL_WATER = 6,
    PAL_FOAM = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
    PAL_STORM = 11,
};

struct Art {
    gs::Mipped hull;
    gs::Mipped rival;
    gs::Mipped cliff;
    gs::Mipped pine;
    gs::Mipped arch;
    gs::Mipped foam;
    gs::Mipped flake;
    gs::Mipped cabin;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargepass
