// S3 SKIFF SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_PIER = 2,
    PAL_PILE = 3,
    PAL_MARK = 4,
    PAL_FOAM = 5,
    PAL_BIRD = 6,
    PAL_TIDE = 7,
    PAL_BUOY = 8,
    PAL_SHED = 9,
    PAL_MAP = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SLIP = 13,
    PAL_SHORE = 14,
    PAL_BANNER = 15
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped pier, head, pile, cleat, ladder, shed, tuft;
    gs::Mipped buoyR, buoyG, flag, staff, bobber;
    gs::Mipped heron[2];
    gs::Mipped gull[2];
    gs::Mipped foam, dash, dot, pin, panel;
    gs::Mipped title, berthed, inSlip, missed, tide, scraped, leg, paused, endMark;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffslip
