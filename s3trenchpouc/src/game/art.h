// S3 TRENCH POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trenchpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_CHALK = 1,
    PAL_KHAKI = 2,
    PAL_POUCH = 3,
    PAL_FLARE = 4,
    PAL_MUD = 5,
    PAL_WIRE = 6,
    PAL_SKY = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, crouch;
    gs::Mipped pouch, coil, stake, burst, plank, bag, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trenchpouc
