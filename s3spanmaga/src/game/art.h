// S3 SPAN MAGA sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spanmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_COAT = 4,
    PAL_COATB = 5,
    PAL_STEEL = 6,
    PAL_PIER = 7,
    PAL_FX = 8,
    PAL_DUSK = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped stand, brace, fallen;
    gs::Mipped truss, pier, lamp, cable, abutment;
    gs::Mipped rifle, sight, round, spent, flash, spark, bar;
    gs::Mipped moon, star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanmaga
