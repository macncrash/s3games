// S3 BEACON BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace beaconbann {

enum Pal {
    PAL_HUD = 0,
    PAL_SEA = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_KEEPER = 4,
    PAL_STONE = 5,
    PAL_LAMP = 6,
    PAL_NIGHT = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, shove;
    gs::Mipped keeper[2];
    gs::Mipped banner;
    gs::Mipped pole;
    gs::Mipped tower;
    gs::Mipped lamp;
    gs::Mipped rock;
    gs::Mipped rail;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace beaconbann
