// S3 BUNKER POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunker {

enum Pal {
    PAL_HUD = 0,
    PAL_CONC = 1,
    PAL_STEEL = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_STEAM = 5,
    PAL_SUMP = 6,
    PAL_ALERT = 7,
    PAL_GO = 8,
    PAL_LAMP = 9
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped crate, rib, slab, door, lamp, floor;
    gs::Mipped pipe, puff[2], sump, grate, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunker
