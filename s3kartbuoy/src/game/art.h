// S3 KARTBUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace buoy {

enum Pal {
    PAL_HUD = 0,
    PAL_WATER = 1,
    PAL_QUAY = 2,
    PAL_KART = 3,
    PAL_RED = 4,
    PAL_GOLD = 5,
    PAL_GREEN = 6,
    PAL_WOOD = 7
};

struct Art {
    gs::Mipped kart[16];
    gs::Mipped buoy;
    gs::Mipped post;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int water = 1;
    int quay = 2;
    int line = 3;
    int dock = 4;
    int check = 5;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace buoy
