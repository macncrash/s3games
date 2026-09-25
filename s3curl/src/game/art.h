// S3 CURL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace curl {

enum Pal {
    PAL_ICE = 0,
    PAL_RED = 1,
    PAL_YEL = 2,
    PAL_HUD = 3,
    PAL_GOLD = 4,
    PAL_DIM = 5,
    PAL_ALERT = 6,
    PAL_BROOM = 7,
    PAL_DOT = 8,
    PAL_WIN = 9,
    PAL_PUFF = 10,
    PAL_SKIP = 11
};

struct Art {
    gs::Image rink;
    gs::Mipped stone;
    gs::Mipped broom;
    gs::Mipped puff;
    gs::Mipped skip;
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curl
