// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace wicketmark {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GREEN = 2,
    PAL_BAD = 3,
    PAL_BALL = 4,
    PAL_COIN = 5,
    PAL_WOOD = 6,
    PAL_WHITE = 7,
    PAL_BOWL = 8,
    PAL_GRASS = 9,
    PAL_SKY = 10,
    PAL_STAND = 11,
    PAL_PITCH = 12,
    PAL_TITLE = 13,
    PAL_KEEP = 14,
    PAL_SHADE = 15
};

struct Art {
    int font[96] = {};
    gs::Image bowler[3];
    gs::Image batsman[2];
    gs::Image keeper;
    gs::Image stump[2];
    gs::Image bail;
    gs::Image ball[2];
    gs::Image coin;
    gs::Image pitch;
    gs::Image tree;
    gs::Image stand;
    gs::Image crowd;
    gs::Image sun;
    gs::Image cloud;
    gs::Image screen;
    gs::Image crease;
    gs::Image scratch;
    gs::Image shadow;
    gs::Image tuft;
    gs::Image dot;
    gs::Image logo;
    gs::Image finished;
    gs::Image wicketWord;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wicketmark
