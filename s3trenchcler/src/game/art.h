// S3 TRENCH CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_MAN = 4,
    PAL_MUD = 5,
    PAL_WOOD = 6,
    PAL_STEEL = 7,
    PAL_BAG = 8,
    PAL_WIRE = 9,
    PAL_FX = 10,
    PAL_LIP = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped digger[2];
    gs::Mipped shovel;
    gs::Mipped spoil;
    gs::Mipped crate;
    gs::Mipped coil;
    gs::Mipped bags;
    gs::Mipped plank;
    gs::Mipped shell;
    gs::Mipped post;
    gs::Mipped lip;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped scrape;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tcler
