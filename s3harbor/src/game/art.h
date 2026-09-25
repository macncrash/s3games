// S3 HARBOR sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace harbor {

enum Pal : int {
    PAL_TEXT = 0,
    PAL_BOAT = 1,
    PAL_FORT = 2,
    PAL_SHOT = 3,
    PAL_FX = 4,
    PAL_AMBER = 5,
    PAL_RED = 6,
    PAL_DIM = 7,
    PAL_QUAY = 8,
    PAL_SKY = 9,
    PAL_DEAD = 10,
    PAL_LIT = 11,
    PAL_ROAD = 12,
};

struct Art {
    gs::Mipped boat, fort, rubble, tower;
    gs::Image shell, hot, burst, splash, puff, flash, reticle, wake, buoy, lamp, sun, cloud, plate;
    gs::Image glyph[128];
    int cellW = 13;
    int cellH = 16;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harbor
