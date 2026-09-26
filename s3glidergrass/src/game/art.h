// S3 GLIDER GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ggrass {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_RIVAL = 5,
    PAL_FIELD = 6,
    PAL_TREE = 7,
    PAL_PROP = 8,
    PAL_SKY = 9,
    PAL_FAR = 10,
    PAL_DUST = 11,
    PAL_BIRD = 12
};

struct Ship {
    gs::Mipped img;
    float ax = 0, ay = 0;  // wheel contact, in source pixels
    float ppm = 14;
};

struct Art {
    Ship ship[5];
    gs::Mipped grass[2];
    gs::Mipped woods, gravel, creek, bank, white;
    gs::Mipped pine, oak, tuft, reed;
    gs::Mipped barn, fence, sign;
    gs::Mipped sock[3];
    gs::Mipped cloud, sun, hill;
    gs::Mipped dust, shade;
    gs::Mipped bird[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ggrass
