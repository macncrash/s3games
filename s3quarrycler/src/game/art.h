// S3 QUARRY CLER sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace qcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_LOADER = 4,
    PAL_ROCK = 5,
    PAL_ORE = 6,
    PAL_STEEL = 7,
    PAL_WALL = 8,
    PAL_DUST = 9,
    PAL_BELT = 10,
    PAL_CAB = 11
};

struct Art {
    gs::Mipped loader[2];
    gs::Mipped bucket[2];
    gs::Mipped slab;
    gs::Mipped boulder;
    gs::Mipped spoil;
    gs::Mipped ore;
    gs::Mipped block;
    gs::Mipped chunk;
    gs::Mipped crusher;
    gs::Mipped hopper;
    gs::Mipped belt;
    gs::Mipped face;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace qcler
