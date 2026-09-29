// S3 CAUSEWAY PACE sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace causewaypace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_COAT = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_TIDE = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_POST = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped coat[2];
    gs::Mipped fallen;
    gs::Mipped post;
    gs::Mipped reed;
    gs::Mipped lantern;
    gs::Mipped heron[2];
    gs::Mipped bead;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped spray;
    gs::Mipped sun;
    gs::Mipped bank;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace causewaypace
