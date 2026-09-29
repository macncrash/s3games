// S3 MAIL VAN PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_VAN = 4,
    PAL_DOCK = 5,
    PAL_ROAD = 6,
    PAL_SACK = 7,
    PAL_OFFICE = 8,
    PAL_SKY = 9,
    PAL_MARK = 10,
    PAL_LAMP = 11,
    PAL_WHEEL = 12,
    PAL_BIRD = 13,
    PAL_BOX = 14,
    PAL_STRIPE = 15
};

struct Art {
    gs::Mipped van, wheel, sack, dock, stripe, road;
    gs::Mipped office, lamp, cloud, bird[2], crate;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailplat
