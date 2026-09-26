// S3 CURL GOLD pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace curlgold {

enum Pal {
    PAL_ICE = 0,
    PAL_RED = 1,
    PAL_YEL = 2,
    PAL_HUD = 3,
    PAL_GOLD = 4,
    PAL_CREAM = 5,
    PAL_DIM = 6,
    PAL_ALERT = 7,
    PAL_WIN = 8,
    PAL_BROOM = 9,
    PAL_DOT = 10,
    PAL_PUFF = 11,
    PAL_SKIP = 12
};

struct Art {
    gs::Image rink;
    gs::Mipped stone;
    gs::Mipped broom;
    gs::Mipped puff;
    gs::Mipped skip;
    gs::Mipped badge2;
    gs::Mipped badge1;
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curlgold
