// S3 RAILBOOM sprites. Everything is drawn at boot. There are no asset files.
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
    PAL_DRIVE = 6,
    PAL_BOOM = 7,
    PAL_IRON = 8,
    PAL_HILL = 9
};

struct Art {
    gs::Mipped cart;
    gs::Mipped cartDuck;
    gs::Mipped rival;
    gs::Mipped drive;
    gs::Mipped boom;
    gs::Mipped beam;
    gs::Mipped sleeper;
    gs::Mipped rail;
    gs::Mipped hill;
    gs::Mipped spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rail
