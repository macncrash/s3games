// S3 ORCHARD PACE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchard {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_LEAF = 4,
    PAL_PICK = 5,
    PAL_SIGHT = 6,
    PAL_APPLE = 7,
    PAL_BARK = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_POST = 11,
    PAL_ROW = 12
};

struct Art {
    gs::Mipped step[2];
    gs::Mipped down;
    gs::Mipped tree;
    gs::Mipped apple;
    gs::Mipped basket;
    gs::Mipped shed;
    gs::Mipped stake;
    gs::Mipped bead;
    gs::Mipped sling;
    gs::Mipped pip;
    gs::Mipped flare;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchard
