// S3 RIDGE CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_WOOD = 6,
    PAL_EARTH = 7,
    PAL_PINE = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped worker[2];
    gs::Mipped barrow;
    gs::Mipped load;
    gs::Mipped rake;
    gs::Mipped brush;
    gs::Mipped stone;
    gs::Mipped crate;
    gs::Mipped crib;
    gs::Mipped rubble;
    gs::Mipped swept;
    gs::Mipped pine;
    gs::Mipped crag;
    gs::Mipped bush;
    gs::Mipped grass;
    gs::Mipped cairn;
    gs::Mipped post;
    gs::Mipped pennant[2];
    gs::Mipped flame;
    gs::Mipped peak;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped hawk[2];
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped pip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rcler
