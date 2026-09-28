// S3 TOWER COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace twc {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CAR = 4,
    PAL_TRUCK = 5,
    PAL_STONE = 6,
    PAL_TREE = 7,
    PAL_FX = 8,
    PAL_LAMP = 9,
    PAL_RED = 10,
    PAL_WHITE = 11,
    PAL_ROAD = 12,
    PAL_FLAG = 13
};

struct Art {
    gs::Mipped car, truck, tower, lamp, link, tree, dust, shadow, cloud, pennant;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace twc
