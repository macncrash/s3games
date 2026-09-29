// S3 OVENTAPE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace oventape {

enum Pal {
    PAL_INK = 0,
    PAL_OVEN = 1,
    PAL_LOAF = 2,
    PAL_GOLD = 3,
    PAL_FLAME = 4,
    PAL_CREAM = 5,
    PAL_BAD = 6,
    PAL_GOOD = 7,
    PAL_TITLE = 8,
    PAL_BAKER = 9,
    PAL_WOOD = 10,
    PAL_TAPE = 11
};

struct Art {
    gs::Mipped oven, loaf, flame, head, drawer, slip;
    gs::Image title, tapeWord, leave;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace oventape
