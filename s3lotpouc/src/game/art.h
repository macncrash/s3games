// S3 LOT POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lot {

enum Pal {
    PAL_HUD = 0,
    PAL_LOT = 1,
    PAL_SKY = 2,
    PAL_PLAYER = 3,
    PAL_CAR = 4,
    PAL_POUCH = 5,
    PAL_BOOTH = 6,
    PAL_FX = 7
};

struct Art {
    gs::Mipped stand, runA, runB, leap;
    gs::Mipped pouch;
    gs::Mipped car, carB;
    gs::Mipped booth, lamp, cone;
    gs::Mipped title, sub;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lot
