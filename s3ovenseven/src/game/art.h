// S3 OVEN SEVEN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ovenseven {

enum Pal {
    PAL_INK = 0,
    PAL_OVEN = 1,
    PAL_RIVAL = 2,
    PAL_LOAF = 3,
    PAL_GOLD = 4,
    PAL_FLAME = 5,
    PAL_CREAM = 6,
    PAL_BAD = 7,
    PAL_GOOD = 8,
    PAL_TITLE = 9,
    PAL_BAKER = 10
};

struct Art {
    gs::Mipped oven, loaf, flame, head, pip, rack;
    gs::Image title, seven, leave;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ovenseven
