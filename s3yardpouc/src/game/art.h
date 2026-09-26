// S3 YARD POUC pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yardpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_IRON = 1,
    PAL_EARTH = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_CUT = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_HOUSE = 9
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped crate, drum, barrel, plank;
    gs::Mipped boxcar;
    gs::Mipped hook, chain, mast, beam;
    gs::Mipped house, shutter, clerk;
    gs::Mipped buck, lensR, lensG;
    gs::Mipped lamp, flame[2];
    gs::Mipped chev, tower, shed, bell, stake, lever;
    gs::Mipped hole, water, lip, shadow, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yardpouc
