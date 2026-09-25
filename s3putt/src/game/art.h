// S3 PUTT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace putt {

enum Pal {
    PAL_WHITE = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BALL = 4,
    PAL_CUP = 5,
    PAL_HEDGE = 6,
    PAL_SAND = 7,
    PAL_WATER = 8,
    PAL_WOOD = 9,
    PAL_TREE = 10,
    PAL_AIM = 11,
    PAL_LOGO = 12,
    PAL_SUN = 13
};

struct Art {
    gs::Mipped ball;
    gs::Mipped shadow;
    gs::Mipped cup;
    gs::Mipped flag[2];
    gs::Mipped hedge;
    gs::Mipped sand;
    gs::Mipped water[2];
    gs::Mipped wood;
    gs::Mipped tree;
    gs::Mipped sun;
    gs::Mipped tuft;
    gs::Mipped dot;
    gs::Mipped logo;
    gs::Mipped win;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace putt
