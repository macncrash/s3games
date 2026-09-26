// S3 RIDGE COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rcol {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_SCOUT = 5,
    PAL_TRUCK = 6,
    PAL_CHAIN = 7,
    PAL_FX = 8,
    PAL_STONE = 9,
    PAL_BANNER = 10,
    PAL_MOUNT = 11,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped warden[2];
    gs::Mipped brace;
    gs::Mipped scout;
    gs::Mipped truck;
    gs::Mipped pennant;
    gs::Mipped chain;
    gs::Mipped post;
    gs::Mipped cairn;
    gs::Mipped banner;
    gs::Mipped lamp;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped peak[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rcol
