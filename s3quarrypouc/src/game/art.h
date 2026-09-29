// S3 QUARRY POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarry {

enum Pal {
    PAL_HUD = 0,
    PAL_ROCK = 1,
    PAL_BENCH = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_IRON = 5,
    PAL_DUST = 6,
    PAL_LAMP = 7,
    PAL_ALERT = 8,
    PAL_PIT = 9,
    PAL_GATE = 10,
    PAL_GO = 11
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped face, skip, cable, beam, lip, ramp;
    gs::Mipped gate, lamp, flame[2], shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarry
