// S3 SKATE SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skateseven {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SKATER = 4,
    PAL_DECK = 5,
    PAL_RIVAL = 6,
    PAL_CITY = 7,
    PAL_PROP = 8,
    PAL_FX = 9
};

struct Art {
    gs::Mipped ride;
    gs::Mipped air;
    gs::Mipped bail;
    gs::Mipped deck;
    gs::Mipped cone;
    gs::Mipped building;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped shadow;
    gs::Mipped logo;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skateseven
