// S3 LOT COLUMN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lotc {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_VAN = 4,
    PAL_SHOP = 5,
    PAL_BOOM = 6,
    PAL_BOOTH = 7,
    PAL_LAMP = 8,
    PAL_STRIPE = 9,
    PAL_FX = 10,
    PAL_ARM = 11,
    PAL_ROAD = 12,
    PAL_SIGN = 13,
    PAL_TREE = 14
};

struct Art {
    gs::Mipped van, shopper;
    gs::Mipped post, arm, lamp;
    gs::Mipped booth, cone, tree;
    gs::Mipped puff, shadow, sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotc
