// S3 GLIDER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace glider {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_TREE = 5,
    PAL_GRASS = 6,
    PAL_ROCK = 7,
    PAL_FIELD = 8,
    PAL_SKY = 9,
    PAL_FAR = 10,
    PAL_LIFT = 11,
    PAL_PROP = 12,
    PAL_BIRD = 13
};

struct Art {
    gs::Mipped ship[5];
    gs::Mipped pine, broad;
    gs::Mipped grass, rock, field;
    gs::Mipped cloud, sun, hill;
    gs::Mipped chevron, shade, puff, gate;
    gs::Mipped sock[3];
    gs::Mipped barn;
    gs::Mipped bird[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace glider
