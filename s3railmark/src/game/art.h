// S3 RAILMARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rail {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_CART = 4,
    PAL_RIVAL = 5,
    PAL_MARK = 6,
    PAL_IRON = 7,
    PAL_HILL = 8,
    PAL_POST = 9
};

struct Art {
    gs::Mipped cart;
    gs::Mipped rival;
    gs::Mipped sleeper;
    gs::Mipped rail;
    gs::Mipped hill;
    gs::Mipped mark;
    gs::Mipped post;
    gs::Mipped spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rail
