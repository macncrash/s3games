// S3 BOCCE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bocce {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_YOU = 2,
    PAL_THEM = 3,
    PAL_JACK = 4,
    PAL_WOOD = 5,
    PAL_GRAVEL = 6,
    PAL_TREE = 7,
    PAL_SUN = 8,
    PAL_AIM = 9,
    PAL_LOGO = 10,
    PAL_AWN = 11,
    PAL_POT = 12,
    PAL_TUFT = 13
};

struct Art {
    gs::Mipped ball;
    gs::Mipped jack;
    gs::Mipped shadow;
    gs::Mipped tree;
    gs::Mipped sun;
    gs::Mipped awning;
    gs::Mipped pot;
    gs::Mipped tuft;
    gs::Mipped dot;
    gs::Mipped logo;
    gs::Mipped win;
    int gravel = 1;
    int wood = 1;
    int chalk = 1;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bocce
