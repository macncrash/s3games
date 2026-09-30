// S3 TRENCH PACE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trenchpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_COAT = 5,
    PAL_SIGHT = 6,
    PAL_LIVE = 7,
    PAL_WATER = 8,
    PAL_FX = 9,
    PAL_MOSS = 10,
    PAL_IRON = 11
};

struct Art {
    gs::Mipped step[2];
    gs::Mipped sunk;
    gs::Mipped block;
    gs::Mipped water;
    gs::Mipped bucket;
    gs::Mipped rope;
    gs::Mipped lamp;
    gs::Mipped bead;
    gs::Mipped rifle;
    gs::Mipped pip;
    gs::Mipped flare;
    gs::Mipped splash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trenchpace
