// S3 SPAN CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spancler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STEEL = 4,
    PAL_FIGURE = 5,
    PAL_WOOD = 6,
    PAL_DECK = 7,
    PAL_WATER = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped hand[2];
    gs::Mipped broom;
    gs::Mipped barrel;
    gs::Mipped crate;
    gs::Mipped coil;
    gs::Mipped patch;
    gs::Mipped tower;
    gs::Mipped lamp;
    gs::Mipped cable;
    gs::Mipped gull[2];
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped splash;
    gs::Mipped shadow;
    gs::Mipped pip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spancler
