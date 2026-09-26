// S3 GATE MAGA sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace maga {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_MAN = 4,
    PAL_MANB = 5,
    PAL_WOOD = 6,
    PAL_STONE = 7,
    PAL_FX = 8,
    PAL_NIGHT = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped stand, crouch, fallen;
    gs::Mipped wall, pole, post, beam, bag;
    gs::Mipped rifle, sight, round, spent, flash, spark;
    gs::Mipped moon, lantern, glow, star, bar;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maga
