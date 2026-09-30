// S3 LENSBELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lensbell {

enum Pal {
    PAL_HUD = 0,
    PAL_BRASS = 1,
    PAL_BELL = 2,
    PAL_GLASS = 3,
    PAL_PAPER = 4,
    PAL_ALERT = 5,
    PAL_OK = 6,
    PAL_STONE = 7
};

struct Art {
    gs::Mipped bell, clapper, rope, beam;
    gs::Mipped body, barrel, glass;
    gs::Mipped corner, caret, notch, plate, haze;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lensbell
