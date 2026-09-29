// S3 QUARRY PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarrypace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_WOOD = 8,
    PAL_FX = 9,
    PAL_ORE = 10,
    PAL_METAL = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped walk[2];
    gs::Mipped fallen;
    gs::Mipped face;
    gs::Mipped derrick;
    gs::Mipped sign;
    gs::Mipped lamp;
    gs::Mipped stake[3];
    gs::Mipped truck;
    gs::Mipped hopper;
    gs::Mipped keg;
    gs::Mipped post;
    gs::Mipped crusher;
    gs::Mipped hawk;
    gs::Mipped plunger;
    gs::Mipped bead;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarrypace
