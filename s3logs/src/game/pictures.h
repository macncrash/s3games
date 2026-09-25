// S3 LOGS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace logs {

enum Pal {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_LOG = 2,
    PAL_ROCK = 3,
    PAL_TREE = 4,
    PAL_BOOM = 5,
    PAL_FX = 6,
    PAL_SKY = 7,
    PAL_TITLE = 8,
    PAL_AMBER = 9,
    PAL_ALERT = 10,
    PAL_GOOD = 11,
    PAL_ROAD = 12,
    PAL_DIM = 13,
};

struct Art {
    gs::Mipped boat[2];
    gs::Mipped stick[3];
    gs::Mipped rock, sweeper;
    gs::Mipped tree[2];
    gs::Mipped piling, wing;
    gs::Mipped mill[2];
    gs::Mipped cabin;
    gs::Mipped splash, shadow, sun, cloud, bird;
    gs::Mipped title, line1, line2, win, fail, sign;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace logs
