// S3 GLIDER PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gliderpass {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_ROCK = 5,
    PAL_SNOW = 6,
    PAL_PINE = 7,
    PAL_STORM = 8,
    PAL_TAPE = 9,
    PAL_HUT = 10,
    PAL_SKY = 11,
    PAL_BIRD = 12,
    PAL_FX = 13,
    PAL_WATER = 14,
    PAL_FLAG = 15
};

struct Wing {
    gs::Mipped img;
    float ax = 0, ay = 0;
    float ppm = 16;
};

struct Art {
    Wing wing[5];
    gs::Mipped rock, snow, grass, pine, tarn, deep;
    gs::Mipped cloud, nimbus, wall, rain, peak, sun;
    gs::Mipped post, tape, hut, cairn, sock, pennant;
    gs::Mipped puff, shadow, gull[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gliderpass
