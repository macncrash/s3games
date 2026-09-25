// S3 SKATE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skate {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SKATER = 4,
    PAL_DECK = 5,
    PAL_RAIL = 6,
    PAL_CITY = 7,
    PAL_PROP = 8,
    PAL_FX = 9,
    PAL_PAD = 10
};

struct Art {
    gs::Mipped ride;
    gs::Mipped air;
    gs::Mipped noseUp;
    gs::Mipped noseDown;
    gs::Mipped grind;
    gs::Mipped bail;
    gs::Mipped deck;
    gs::Mipped pad;
    gs::Mipped rail;
    gs::Mipped stair;
    gs::Mipped bank;
    gs::Mipped block;
    gs::Mipped cone;
    gs::Mipped can;
    gs::Mipped building;
    gs::Mipped tower;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped spark;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped logo;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skate
