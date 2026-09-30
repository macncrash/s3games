// S3 BIKE PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikeplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_BIKE = 4,
    PAL_DECK = 5,
    PAL_PATH = 6,
    PAL_POST = 7,
    PAL_TREE = 8,
    PAL_SKY = 9,
    PAL_MARK = 10,
    PAL_LAMP = 11,
    PAL_DUST = 12,
    PAL_RIDER = 13,
    PAL_WHEEL = 14,
    PAL_SIGN = 15
};

struct Art {
    gs::Mipped body[3];
    gs::Mipped wheel;
    gs::Mipped plank, stripe, post, path;
    gs::Mipped tree, cloud, sun, lamp, dust, sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikeplat
