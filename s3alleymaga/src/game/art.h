// S3 ALLEY MAGA sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alleymaga {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_MAN = 4,
    PAL_MANB = 5,
    PAL_BRICK = 6,
    PAL_IRON = 7,
    PAL_FX = 8,
    PAL_LAMP = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped stand, crouch, fallen;
    gs::Mipped brick, bin, escape, lamp, glow, rifle, sight;
    gs::Mipped round, spent, flash, spark, bar, puddle;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alleymaga
