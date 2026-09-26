// S3 GLIDER SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gslip {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_RIVAL = 5,
    PAL_HARBOR = 6,
    PAL_WOOD = 7,
    PAL_TOWN = 8,
    PAL_SKY = 9,
    PAL_FAR = 10,
    PAL_SPRAY = 11,
    PAL_GULL = 12
};

// Contact point is the keel step, in source pixels.
struct Ship {
    gs::Mipped img;
    float ax = 0, ay = 0;
    float ppm = 13;
};

struct Art {
    Ship boat[5];
    gs::Mipped skiff;
    gs::Mipped bay[2];
    gs::Mipped slipW[2];
    gs::Mipped mud, rock, bluff, plank;
    gs::Mipped pier, dolphin;
    gs::Mipped quay, light, buoy, pennant[3], sign, staff;
    gs::Mipped gull[2], cloud, sun, head;
    gs::Mipped spray, shade;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gslip
