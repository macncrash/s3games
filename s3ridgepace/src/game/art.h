// S3 RIDGE PACE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_PINE = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_WOOD = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped scout[2];
    gs::Mipped fallen;
    gs::Mipped cairn[2];
    gs::Mipped rag;
    gs::Mipped pine;
    gs::Mipped crag;
    gs::Mipped bush;
    gs::Mipped grass;
    gs::Mipped stake;
    gs::Mipped bead;
    gs::Mipped rifle;
    gs::Mipped post;
    gs::Mipped pennant[2];
    gs::Mipped lip;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped peak;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped hawk[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rpace
