// S3 CISTERN COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cistern {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_TRUCK = 4,
    PAL_CAR = 5,
    PAL_STONE = 6,
    PAL_WATER = 7,
    PAL_TREE = 8,
    PAL_FX = 9,
    PAL_GATE = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped truck, car, cistern, gate, spout, reed;
    gs::Mipped tree, dust, wheel;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cistern
