// S3 REDOUBT PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace redoubtpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_EARTH = 1,
    PAL_YOU = 2,
    PAL_LIGHT = 3,
    PAL_HEAVY = 4,
    PAL_BOLT = 5,
    PAL_FX = 6,
    PAL_WRECK = 7,
    PAL_OK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped berm;
    gs::Mipped you[2];
    gs::Mipped light[2];
    gs::Mipped heavy[2];
    gs::Mipped wreck;
    gs::Mipped bolt;
    gs::Mipped puff;
    gs::Mipped stake;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace redoubtpurs
