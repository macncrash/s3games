// S3 BUSTURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace busturn {

enum Pal {
    PAL_HUD = 0,
    PAL_SIGN = 1,
    PAL_BUS = 2,
    PAL_CREW = 3,
    PAL_TOWN = 4,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bus[5];
    gs::Mipped crew;
    gs::Mipped block[3];
    gs::Mipped tree;
    gs::Mipped arrow;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace busturn
