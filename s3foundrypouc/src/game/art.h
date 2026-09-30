// S3 FOUNDRY POUC sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundrypouc {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_IRON = 2,
    PAL_POUCH = 3,
    PAL_WORKER = 4,
    PAL_SLAG = 5,
    PAL_FURNACE = 6,
    PAL_LADLE = 7,
    PAL_SPARK = 8,
    PAL_DOOR = 9,
    PAL_SOOT = 10
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped furnace, ladle, door, stack, pipe, spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundrypouc
