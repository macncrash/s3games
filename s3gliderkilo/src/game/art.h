// S3 GLIDER KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_MILL = 5,
    PAL_CART = 6,
    PAL_HANG = 7,
    PAL_GRASS = 8,
    PAL_HILL = 9,
    PAL_SKY = 10,
    PAL_TREE = 11,
    PAL_BANNER = 12,
    PAL_POST = 13,
    PAL_SHADE = 14
};

struct Ship {
    gs::Mipped img;
    float ax = 0, ay = 0;
    float ppm = 11;
};

struct Art {
    Ship ship[5];
    gs::Mipped wheel[4];
    float wheelRim = 0.9f;  // rim diameter as a fraction of the bitmap height
    gs::Mipped tower;
    float towerAx = 0, towerAy = 0, towerSpan = 1;
    gs::Mipped bed, beam, cable;
    gs::Mipped grass, hill, cloud, deck, sun;
    gs::Mipped tree, bird[2], sock[2];
    gs::Mipped pole, banner, shade;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gkilo
