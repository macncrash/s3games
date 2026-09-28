// S3 MILL DOOR pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_TEXT = 0,
    PAL_LAMP = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_OAK = 5,
    PAL_IRON = 6,
    PAL_WATER = 7,
    PAL_FLOUR = 8,
    PAL_SKY = 9,
    PAL_WHEEL = 10,
    PAL_APRON = 11,
    PAL_RACE = 12
};

struct Art {
    gs::Mipped leaf;
    gs::Mipped post;
    gs::Mipped beam;
    gs::Mipped miller[2];
    gs::Mipped wheel[2];
    gs::Mipped sack;
    gs::Mipped chock;
    gs::Mipped splash;
    gs::Mipped dust;
    gs::Mipped vane;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
