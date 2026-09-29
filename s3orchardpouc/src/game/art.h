// S3 ORCHARD POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchard {

enum Pal {
    PAL_HUD = 0,
    PAL_GRASS = 1,
    PAL_DIRT = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_BARK = 5,
    PAL_LEAF = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_WATER = 9,
    PAL_SHED = 10,
    PAL_GO = 11
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped tree, apple, crate, bough, plank;
    gs::Mipped ditch, shed, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchard
