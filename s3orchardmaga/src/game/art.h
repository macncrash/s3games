// S3 ORCHARD MAGA sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchardmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_MAN = 4,
    PAL_MANB = 5,
    PAL_TREE = 6,
    PAL_WOOD = 7,
    PAL_FX = 8,
    PAL_APPLE = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped stand, crouch, fallen;
    gs::Mipped tree, crate, ladder, lamp, glow, rifle, sight;
    gs::Mipped round, spent, flash, spark, bar, apple;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchardmaga
