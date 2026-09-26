// S3 SLED SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_CRIB = 2,
    PAL_LAMP = 3,
    PAL_MARK = 4,
    PAL_SNOW = 5,
    PAL_BIRD = 6,
    PAL_TIDE = 7,
    PAL_MAP = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
    PAL_BANNER = 11,
    PAL_ICE = 12,
    PAL_BERTH = 13,
    PAL_BANK = 14,
    PAL_HUT = 15
};

struct Art {
    gs::Mipped team[16];
    gs::Mipped crib, head, hut, smoke[2];
    gs::Mipped lamp, raven[2], stake, bar, staff, bead;
    gs::Mipped mound, puff, flake, panel, pin;
    gs::Mipped title, berthed, inSlip, missed, tide, scraped, offIce, leg, paused, endMark;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledslip
