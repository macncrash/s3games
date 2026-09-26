// S3 GLIDER TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gliderturn {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_PYLON = 5,
    PAL_GRASS = 6,
    PAL_PINE = 7,
    PAL_ROCK = 8,
    PAL_SKY = 9,
    PAL_END = 10,
    PAL_FAR = 11,
    PAL_HOUSE = 12,
    PAL_DUST = 13,
    PAL_FIELD = 14,
    PAL_CHEV = 15
};

struct Ship {
    gs::Mipped img;
    float ax = 0, ay = 0;  // cg in the cropped bitmap
    float ppm = 14;
};

struct Art {
    Ship ship[7];
    gs::Mipped pole, flag, arm, banner, post, check, stripe;
    gs::Mipped grass, pine, rock, cloud, sun, ridge;
    gs::Mipped gull[2], dust, shade, bothy, sock[3], chev;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gliderturn
