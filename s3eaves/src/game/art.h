// S3 EAVES sprites and tiles. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace eaves {

enum Pal {
    PAL_HUD = 0,
    PAL_CITY = 1,
    PAL_PLAYER = 2,
    PAL_IRON = 3,
    PAL_BRASS = 4,
    PAL_BIRD = 5,
    PAL_FX = 6,
    PAL_AMBER = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_FAR = 10,
    PAL_MOON = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, bird[2], moon, cloud, hatch, lantern, chimney, dust, shadow, streak, wire;
    gs::Mipped glyph[96];
    int font[96] = {};
    int slate = 1, slateB = 1, capL = 1, capR = 1, gutter = 1;
    int brick = 1, brickB = 1, window = 1, windowD = 1, ivy = 1, tower = 1;
    int star = 1, starB = 1, farWall = 1, farRoof = 1, farChim = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eaves
