// S3 CLIFF LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace clifflock {

enum Pal {
    PAL_HUD = 0,
    PAL_ROCK = 1,
    PAL_TRUCK = 2,
    PAL_GATE = 3,
    PAL_SEA = 4,
    PAL_CREW = 5,
    PAL_INK = 6,
    PAL_WARN = 7
};

struct Art {
    gs::Mipped truck;
    gs::Mipped slab;
    gs::Mipped cliff;
    gs::Mipped leaf;
    gs::Mipped post;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clifflock
