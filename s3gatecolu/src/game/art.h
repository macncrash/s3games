// S3 GATE COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace colu {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CAR = 4,
    PAL_VAN = 5,
    PAL_TRUCK = 6,
    PAL_POST = 7,
    PAL_BOOTH = 8,
    PAL_TREE = 9,
    PAL_FX = 10,
    PAL_RED = 11,
    PAL_ROAD = 12,
    PAL_WHITE = 13,
    PAL_FLAG = 14
};

struct Art {
    gs::Mipped car, van, truck;
    gs::Mipped booth, post, stripe, lamp;
    gs::Mipped tree[2];
    gs::Mipped sign, pennant;
    gs::Mipped dust, shadow, cloud, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace colu
