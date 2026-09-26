// S3 PUTTBELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace puttbell {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GREEN = 3,
    PAL_BALL = 4,
    PAL_BELL = 5,
    PAL_WOOD = 6,
    PAL_CUP = 7,
    PAL_TREE = 8,
    PAL_AIM = 9,
    PAL_PLAYER = 10,
    PAL_GRASS = 11,
    PAL_CLOUD = 12,
    PAL_SUN = 13
};

struct Art {
    gs::Mipped ball;
    gs::Mipped shadow;
    gs::Mipped cup;
    gs::Mipped bell;
    gs::Mipped clapper;
    gs::Mipped yoke;
    gs::Mipped golfer[2];
    gs::Mipped rail;
    gs::Mipped tuft;
    gs::Mipped chevron;
    gs::Mipped tree;
    gs::Mipped flower;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped dot;
    gs::Mipped logo;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace puttbell
