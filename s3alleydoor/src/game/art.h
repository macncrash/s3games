// S3 ALLEY DOOR pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alley {

enum Pal {
    PAL_TEXT = 0,
    PAL_LAMP = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BRICK = 4,
    PAL_COAT = 5,
    PAL_IRON = 6,
    PAL_NEON = 7,
    PAL_PROP = 8,
    PAL_WET = 9,
    PAL_SKY = 10,
    PAL_MIST = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped leaf;
    gs::Mipped jamb;
    gs::Mipped lintel;
    gs::Mipped keeper[2];
    gs::Mipped pusher[2];
    gs::Mipped chain;
    gs::Mipped brick;
    gs::Mipped lamp;
    gs::Mipped neon;
    gs::Mipped bin;
    gs::Mipped cat;
    gs::Mipped pipe;
    gs::Mipped grate;
    gs::Mipped drop;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alley
