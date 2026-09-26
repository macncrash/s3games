// S3 GLIDER PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gliderplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_TIMBER = 5,
    PAL_ROCK = 6,
    PAL_GRASS = 7,
    PAL_PINE = 8,
    PAL_SKY = 9,
    PAL_END = 10,
    PAL_FAR = 11,
    PAL_POST = 12,
    PAL_HOUSE = 13,
    PAL_DUST = 14,
    PAL_SIGN = 15
};

struct Wing {
    gs::Mipped img;
    float ax = 0, ay = 0;  // wheel contact in the cropped bitmap
    float ppm = 12;
};

struct Art {
    Wing wing[5];
    gs::Mipped plank, stripe, trestle;
    gs::Mipped rock, grass, pine;
    gs::Mipped cloud, sun, ridge;
    gs::Mipped sock[3], cabin, sign, lamp, chev;
    gs::Mipped gull[2], dust, shade;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gliderplat
