// S3 MARKET pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace market {

enum Pal {
    PAL_HUD = 0,
    PAL_STALL = 1,
    PAL_CLERK = 2,
    PAL_C0 = 3,
    PAL_C1 = 4,
    PAL_C2 = 5,
    PAL_C3 = 6,
    PAL_GOODS = 7,
    PAL_COIN = 8,
    PAL_BILL = 9,
    PAL_OK = 10,
    PAL_WARN = 11,
    PAL_BAD = 12,
    PAL_DIM = 13
};

struct Art {
    gs::Mipped awning, post, counter, crate;
    gs::Mipped clerk, person, hat, bag;
    gs::Mipped good[8];
    gs::Mipped coin[4];
    gs::Mipped bill, dish, bell, board;
    gs::Image shade;
    gs::Image bracket;
    gs::Image solid;
    gs::Image digit[10];
    gs::Image title, stall, moved, stalled, exact, brief, over, line;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace market
