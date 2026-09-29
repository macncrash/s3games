// S3 CISTERN POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cisternpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WATER = 2,
    PAL_KEEPER = 3,
    PAL_POUCH = 4,
    PAL_ROPE = 5,
    PAL_MOSS = 6
};

struct Art {
    gs::Mipped stand, walkA, walkB;
    gs::Mipped pouch;
    gs::Mipped bucket;
    gs::Mipped drip;
    gs::Mipped lip;
    gs::Mipped watch;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cisternpouc
