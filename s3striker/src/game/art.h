// S3 STRIKER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace striker {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_MAN = 4,
    PAL_WOOD = 5,
    PAL_BRASS = 6,
    PAL_PROP = 7,
    PAL_FX = 8
};

struct Art {
    gs::Mipped man[7];
    gs::Mipped tower;
    gs::Mipped bell[3];
    gs::Mipped puck;
    gs::Mipped flash;
    gs::Mipped spark;
    gs::Mipped ground;
    gs::Mipped tent;
    gs::Mipped lights[2];
    gs::Mipped bunting;
    gs::Mipped person[3];
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped bulb;
    gs::Mipped glyph[96];
    int font[96] = {};

    int footX = 0, footY = 0;
    int shoulderX = 0, shoulderY = 0;
    int towerSlotX = 0, towerSlotTop = 0, towerSlotBot = 0, towerAnvilY = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace striker
