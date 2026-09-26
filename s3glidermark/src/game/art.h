// S3 GLIDER MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gmark {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_GRASS = 5,
    PAL_LAKE = 6,
    PAL_MARK = 7,
    PAL_WOOD = 8,
    PAL_TREE = 9,
    PAL_FAR = 10,
    PAL_SKY = 11,
    PAL_DUST = 12,
    PAL_BOAT = 13
};

struct Ship {
    gs::Mipped img;
    float ax = 0, ay = 0;  // CG, source pixels
    float ppm = 13;
};

struct Art {
    Ship ship[5];
    gs::Mipped grass[2];
    gs::Mipped lake[2];
    gs::Mipped foam;
    gs::Mipped mark;
    gs::Mipped chev;
    gs::Mipped flag[2];
    gs::Mipped sock[3];
    gs::Mipped tower;
    gs::Mipped pine;
    gs::Mipped reed;
    gs::Mipped boat;
    gs::Mipped hill;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped dust;
    gs::Mipped shade;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gmark
