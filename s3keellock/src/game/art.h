// S3 KEEL LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keellock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_SAIL = 2,
    PAL_GATE = 3,
    PAL_STONE = 4,
    PAL_BANK = 5,
    PAL_CREW = 6,
    PAL_FOAM = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
};

struct Art {
    gs::Mipped hull;
    gs::Mipped sail;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped reed;
    gs::Mipped lamp;
    gs::Mipped foam;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keellock
