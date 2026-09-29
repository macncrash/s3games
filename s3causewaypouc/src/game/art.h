// S3 CAUSEWAY POUC sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cwpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_WALKER = 1,
    PAL_POUCH = 2,
    PAL_PLANK = 3,
    PAL_POST = 4,
    PAL_GATE = 5,
    PAL_GULL = 6,
    PAL_ALERT = 7,
    PAL_GOOD = 8,
    PAL_TITLE = 9,
    PAL_WATER = 10
};

struct Art {
    gs::Mipped stand, runA, runB, leap;
    gs::Mipped pouch;
    gs::Mipped plank, post, gate, gull;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cwpouc
