// S3 CHEF GOLD pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace chefgold {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_PAPER = 3,
    PAL_STEEL = 4,
    PAL_FOOD = 5,
    PAL_CHEF = 6,
    PAL_FIRE = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped chef;
    gs::Mipped pan;
    gs::Mipped flame;
    gs::Mipped goldDish;
    gs::Mipped creamDish;
    gs::Mipped ticket;
    gs::Mipped solid;
};

void buildPictures(gs::VDP& vdp, Art& art);

}  // namespace chefgold
