// S3 TUGBOAT SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_PIER = 2,
    PAL_PILE = 3,
    PAL_MARK = 4,
    PAL_FOAM = 5,
    PAL_BIRD = 6,
    PAL_TIDE = 7,
    PAL_BUOY = 8,
    PAL_YARD = 9,
    PAL_SMOKE = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SLIP = 13,
    PAL_SHORE = 14,
    PAL_BANNER = 15
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped pier, head, pile, fender, cleat;
    gs::Mipped nun, can, barge, boat, shed, crane, tuft;
    gs::Mipped gull[2], foam, smoke, ebb, staff, bobber;
    gs::Mipped dash, dot, pin, panel, mark;
    gs::Mipped title, berthed, inSlip, tide, headMsg, scraped, past, shortMsg, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugslip
