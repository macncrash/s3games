// S3 SCORE GOLD pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scoregold {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_CHALK = 2,
    PAL_CREAM = 3,
    PAL_GOLD = 4,
    PAL_BAD = 5,
    PAL_OK = 6,
    PAL_INK = 7
};

struct Art {
    gs::Mipped board, ledge, pen;
    gs::Mipped bare, cream, gold;
    gs::Image solid;
    gs::Image title, rule, filed, shorted;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scoregold
