// S3 TRENCH BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trenchbann {

enum Pal {
    PAL_HUD = 0,
    PAL_TIMBER = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_FLARE = 4,
    PAL_MUD = 5,
    PAL_BAG = 6,
    PAL_SKY = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, crouch;
    gs::Mipped banner;
    gs::Mipped pole;
    gs::Mipped bag;
    gs::Mipped duck;
    gs::Mipped flare;
    gs::Mipped stake;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trenchbann
