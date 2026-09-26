// S3 GLIDER LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gliderlock {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_GATE = 5,
    PAL_STONE = 6,
    PAL_WATER = 7,
    PAL_BANK = 8,
    PAL_HOUSE = 9,
    PAL_SKY = 10,
    PAL_TAPE = 11,
    PAL_BIRD = 12,
    PAL_FX = 13
};

struct Wing {
    gs::Mipped img;
    float ax = 0, ay = 0;  // CG in the cropped bitmap
    float ppm = 20;
};

struct Art {
    Wing wing[5];
    gs::Mipped leaf, pier, lintel, sill;
    gs::Mipped water, deep, grass, stone;
    gs::Mipped reed, willow, cottage, heron;
    gs::Mipped sock[3], cloud, hill, sun;
    gs::Mipped post, bunting, lamp, puff, shadow;
    gs::Mipped gull[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gliderlock
