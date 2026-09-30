// S3 KEEL KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keelkilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_SAIL = 2,
    PAL_WHEEL = 3,
    PAL_MILL = 4,
    PAL_BANK = 5,
    PAL_POST = 6,
    PAL_FOAM = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
};

struct Art {
    gs::Mipped hull;
    gs::Mipped sail;
    gs::Mipped wheel;
    gs::Mipped mill;
    gs::Mipped reed;
    gs::Mipped post;
    gs::Mipped tape;
    gs::Mipped foam;
    gs::Mipped title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keelkilo
