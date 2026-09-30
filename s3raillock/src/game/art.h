// S3 RAIL LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace raillock {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_GATE = 2,
    PAL_STONE = 3,
    PAL_WATER = 4,
    PAL_RAIL = 5,
    PAL_TREE = 6,
    PAL_ALERT = 7
};

struct Art {
    gs::Mipped car;
    gs::Mipped low;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped water;
    gs::Mipped sleeper;
    gs::Mipped tree;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace raillock
