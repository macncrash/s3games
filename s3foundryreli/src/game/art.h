// S3 FOUNDRY RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundry {

enum Pal {
    PAL_HUD = 0,
    PAL_HALL = 1,
    PAL_YOU = 2,
    PAL_CINDER = 3,
    PAL_SLAG = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_GLOW = 9
};

struct Art {
    gs::Mipped hall, crucible, ladle, flash, sight;
    gs::Mipped pourer[2];
    gs::Mipped cinder[2], slag[2], ingot[2];
    gs::Mipped bell, rope;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundry
