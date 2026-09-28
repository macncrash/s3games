// S3 LUGE BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_HUD = 0,
    PAL_INK = 1,
    PAL_AMBER = 2,
    PAL_FX = 3,
    PAL_RIDER = 4,
    PAL_RIVAL = 5,
    PAL_BOOM = 6,
    PAL_TREE = 7,
    PAL_ROCK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped luge[3];
    gs::Mipped rival;
    gs::Mipped boom;
    gs::Mipped tree;
    gs::Mipped rock;
    gs::Mipped shadow;
    gs::Mipped flake;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
