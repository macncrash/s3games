// S3 ARCHMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace archmark {

enum Pal {
    PAL_FACE = 0,
    PAL_ARCH = 1,
    PAL_ARROW = 2,
    PAL_COIN = 3,
    PAL_SIGHT = 4,
    PAL_HAND = 5,
    PAL_WORLD = 6,
    PAL_INK = 7,
    PAL_GOLD = 8,
    PAL_ALERT = 9,
    PAL_TITLE = 10,
    PAL_WIN = 11
};

struct Art {
    gs::Image face;
    gs::Image archer;
    gs::Image arrow;
    gs::Image coin;
    gs::Image hand;
    gs::Image sight;
    gs::Image bale;
    gs::Image tree;
    gs::Image title;
    gs::Image win;
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace archmark
