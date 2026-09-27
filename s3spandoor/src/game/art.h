// S3 SPANDOOR sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spandoor {

enum Pal {
    PAL_HUD = 0,
    PAL_STEEL = 1,
    PAL_WATER = 2,
    PAL_COAT = 3,
    PAL_LAMP = 4,
    PAL_WAGON = 5,
    PAL_CABLE = 6,
    PAL_WARN = 7,
    PAL_GOLD = 8,
    PAL_RED = 9
};

struct Art {
    gs::Mipped gate, tower, deck, cable, lamp;
    gs::Mipped watch, watchLean;
    gs::Mipped wagon, pin, streak, chip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spandoor
