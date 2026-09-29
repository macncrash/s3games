// S3 PALISADE POUC sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisadepouc {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_EARTH = 2,
    PAL_POUCH = 3,
    PAL_RUNNER = 4,
    PAL_SKY = 5,
    PAL_BAR = 6,
    PAL_GO = 7,
    PAL_ALERT = 8
};

struct Art {
    gs::Mipped stand, runA, runB, leap;
    gs::Mipped pouch;
    gs::Mipped stake, post, turf, ditch, bar, mark;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisadepouc
