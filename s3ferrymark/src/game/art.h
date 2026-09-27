// S3 FERRY MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fmark {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_HULL = 4,
    PAL_PIER = 5,
    PAL_MARK = 6,
    PAL_WATER = 7,
    PAL_FOAM = 8,
    PAL_SHED = 9,
    PAL_BUOY = 10,
    PAL_WAKE = 11
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped pier;
    gs::Mipped shed;
    gs::Mipped mark;
    gs::Mipped cross;
    gs::Mipped buoy;
    gs::Mipped foam;
    gs::Mipped wake;
    gs::Mipped post;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fmark
