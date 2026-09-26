// S3 YARD CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yardcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_KEEPER = 4,
    PAL_LEAF = 5,
    PAL_WOOD = 6,
    PAL_TOY = 7,
    PAL_STONE = 8,
    PAL_HOUSE = 9,
    PAL_TREE = 10,
    PAL_FX = 11,
    PAL_LAWN = 12,
    PAL_BIRD = 13,
    PAL_BLOOM = 14
};

struct Art {
    gs::Mipped keeper[3];
    gs::Mipped barrow;
    gs::Mipped leaves, sticks, branch, toys, clippings, stones;
    gs::Mipped raked;
    gs::Mipped bin;
    gs::Mipped house, tree, fence, flower;
    gs::Mipped hose, can, line;
    gs::Mipped bird[2];
    gs::Mipped sun, cloud, clock;
    gs::Mipped dust, shadow, aim;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yardcler
