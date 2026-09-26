// S3 STANDARD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace standard {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BIKE = 4,
    PAL_LORRY = 5,
    PAL_FLAG = 6,
    PAL_TREE = 7,
    PAL_CAMP = 8,
    PAL_FX = 9,
    PAL_HILL = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bike;
    gs::Mipped lorryF, lorryR;
    gs::Mipped carF, carR;
    gs::Mipped flag[2];
    gs::Mipped poplar, tree;
    gs::Mipped post, tent, tape;
    gs::Mipped dust, burst, shadow, cloud, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
    int hill = 1;
    int hillHi = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace standard
