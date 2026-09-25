// GIGaBOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace gig {

enum Pal {
    PAL_WHITE = 0,
    PAL_GOLD = 1,
    PAL_GREEN = 2,
    PAL_CYAN = 3,
    PAL_RIDER = 4,
    PAL_GROUND = 5,
    PAL_CAR = 6,
    PAL_NATURE = 7,
    PAL_PICK = 8,
    PAL_MAIL = 9,
    PAL_H0 = 10,
    PAL_H1 = 11,
    PAL_H2 = 12,
    PAL_H3 = 13,
    PAL_ORANGE = 14,
    PAL_RED = 15
};

struct Art {
    gs::Mipped glyph[96];
    gs::Mipped letterA;
    gs::Mipped ground;
    gs::Mipped house[2];
    gs::Mipped rider, ring, mat, matOff;
    gs::Mipped mail, tree, lamp, crate, hole, car, cup, blob;
    gs::Mipped button, shade;
    int font[96] = {};
    int tileBar = 0, tileBarDim = 0, tileStar = 0, tileStarDim = 0, tileDollar = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gig
