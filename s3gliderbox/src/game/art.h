// S3 GLIDER BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gbox {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_SALT = 5,
    PAL_PAD = 6,
    PAL_POST = 7,
    PAL_FAR = 8,
    PAL_SKY = 9,
    PAL_DUST = 10,
    PAL_SIGN = 11
};

struct Ship {
    gs::Mipped img;
    float ax = 0, ay = 0;  // CG, in source pixels
    float ppm = 12;
};

struct Art {
    Ship ship[5];
    gs::Mipped salt, pad, edge;
    gs::Mipped post[2];
    gs::Mipped sock[3];
    gs::Mipped mesa, cloud, sun, bush;
    gs::Mipped chev, dust, shade;
    gs::Mipped sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gbox
