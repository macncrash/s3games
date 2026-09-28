// S3 MILL PACE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_REED = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_WOOD = 11,
    PAL_ROAD = 12,
    PAL_MILL = 13,
    PAL_WATER = 14
};

struct Art {
    gs::Mipped miller[2];
    gs::Mipped fallen;
    gs::Mipped sack;
    gs::Mipped flag;
    gs::Mipped reed;
    gs::Mipped mill;
    gs::Mipped wheel[2];
    gs::Mipped bush;
    gs::Mipped grass;
    gs::Mipped stake;
    gs::Mipped bead;
    gs::Mipped rifle;
    gs::Mipped post;
    gs::Mipped vane[2];
    gs::Mipped lip;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped roof;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped duck[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
