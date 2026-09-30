// S3 TILE SEVEN pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tileseven {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WALL = 4,
    PAL_CLAY = 5,
    PAL_GLAZE = 6,
    PAL_INK = 7,
    PAL_IVORY = 8
};

struct Art {
    gs::Mipped tile;
    gs::Mipped pip;
    gs::Mipped crack;
    gs::Mipped niche;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tileseven
