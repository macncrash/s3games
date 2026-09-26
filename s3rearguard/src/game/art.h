// S3 REARGUARD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rearguard {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_PLAYER = 4,
    PAL_FRIEND = 5,
    PAL_ENEMY = 6,
    PAL_HORSE = 7,
    PAL_FX = 8,
    PAL_PROP = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped player[2];
    gs::Mipped file[2];
    gs::Mipped rifle[2];
    gs::Mipped runner;
    gs::Mipped horse;
    gs::Mipped officer;
    gs::Mipped wagon;
    gs::Mipped gate;
    gs::Mipped tree;
    gs::Mipped bush;
    gs::Mipped cloud;
    gs::Mipped flash;
    gs::Mipped puff;
    gs::Mipped shot;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rearguard
