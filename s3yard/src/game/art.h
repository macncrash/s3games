// S3 YARD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yard {

enum Pal : int {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_BLUE = 2,
    PAL_OLIVE = 3,
    PAL_CREAM = 4,
    PAL_WRECK = 5,
    PAL_SCRAP = 6,
    PAL_PURSE = 7,
    PAL_FX = 8,
    PAL_GROUND = 9,
    PAL_FENCE = 10,
    PAL_ALERT = 11,
    PAL_OK = 12,
    PAL_PRIZE = 13,
    PAL_SIGN = 14,
};

struct Art {
    gs::Mipped car[8];
    gs::Mipped wreck;
    gs::Mipped pile[2];
    gs::Mipped drum;
    gs::Mipped crane;
    gs::Mipped purse;
    gs::Mipped sign;
    gs::Mipped smoke;
    gs::Mipped spark;
    gs::Mipped flame;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yard
