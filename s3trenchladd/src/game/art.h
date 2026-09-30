// S3 TRENCH LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trenchladd {

enum Pal {
    PAL_HUD = 0,
    PAL_CHALK = 1,
    PAL_KHAKI = 2,
    PAL_LADDER = 3,
    PAL_TRACE = 4,
    PAL_MUD = 5,
    PAL_IRON = 6,
    PAL_SKY = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, crouch, leap;
    gs::Mipped ladder, rung, crater, post, tracer, bag, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trenchladd
