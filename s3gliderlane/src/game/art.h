// S3 GLIDER LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace glane {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_CREW = 5,
    PAL_RAIL = 6,
    PAL_TIGHT = 7,
    PAL_BANK = 8,
    PAL_WATER = 9,
    PAL_REED = 10,
    PAL_SKY = 11,
    PAL_HILL = 12,
    PAL_SHED = 13,
    PAL_DUST = 14,
    PAL_SIGN = 15
};

struct Wing {
    gs::Mipped img;
    float ax = 0, ay = 0;
    float ppm = 16;
};

struct Art {
    Wing wing[5];
    gs::Mipped rail, post, flag, chev;
    gs::Mipped signLane, signEnd;
    gs::Mipped shed, sock[3];
    gs::Mipped reed, bank, water, hill;
    gs::Mipped cloud, sun, gull[2];
    gs::Mipped dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace glane
