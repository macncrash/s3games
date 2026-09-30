// S3 KEEL MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keelmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_BANK = 2,
    PAL_WATER = 3,
    PAL_MARK = 4,
    PAL_END = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_WAKE = 8,
    PAL_REED = 9
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped shade;
    gs::Mipped buoy;
    gs::Mipped flag;
    gs::Mipped water, bank, ring, endbar, reed, committee;
    gs::Mipped title, set, missed, hold, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keelmark
