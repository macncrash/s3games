// S3 BUNKER COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bcol {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_TRUCK = 4,
    PAL_CAR = 5,
    PAL_BUNKER = 6,
    PAL_BAR = 7,
    PAL_TREE = 8,
    PAL_FX = 9,
    PAL_LAMP = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped truck, car, bunker, slit, bar, stripe;
    gs::Mipped tree, dust, lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bcol
