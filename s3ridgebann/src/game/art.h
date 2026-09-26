// S3 RIDGE BANN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rbann {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_FOE = 5,
    PAL_BANNER = 6,
    PAL_STONE = 7,
    PAL_FX = 8,
    PAL_MOUNT = 9,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped runner[2];
    gs::Mipped plant;
    gs::Mipped foe[2];
    gs::Mipped banner[2];
    gs::Mipped staff;
    gs::Mipped cairn;
    gs::Mipped stone;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped peak[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rbann
