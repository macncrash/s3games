// S3 BEACON PACE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace beacon {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_COAT = 5,
    PAL_SIGHT = 6,
    PAL_LIVE = 7,
    PAL_LAMP = 8,
    PAL_FX = 9,
    PAL_SEA = 10,
    PAL_IRON = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped step[2];
    gs::Mipped down;
    gs::Mipped tower;
    gs::Mipped lantern;
    gs::Mipped flame;
    gs::Mipped rail;
    gs::Mipped spur;
    gs::Mipped bead;
    gs::Mipped carbine;
    gs::Mipped pip;
    gs::Mipped flare;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace beacon
