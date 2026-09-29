// S3 CISTERN BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cisternbann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WATER = 2,
    PAL_KEEPER = 3,
    PAL_BANNER = 4,
    PAL_EEL = 5,
    PAL_MOSS = 6
};

struct Art {
    gs::Mipped stand, walkA, walkB;
    gs::Mipped banner;
    gs::Mipped eel;
    gs::Mipped drip;
    gs::Mipped lip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cisternbann
