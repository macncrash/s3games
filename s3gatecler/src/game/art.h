// S3 GATE CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_EARTH = 6,
    PAL_CRATE = 7,
    PAL_TREE = 8,
    PAL_FX = 9,
    PAL_NIGHT = 10,
    PAL_YARD = 12
};

struct Art {
    gs::Mipped worker[2];
    gs::Mipped barrow;
    gs::Mipped load;
    gs::Mipped brush;
    gs::Mipped stone;
    gs::Mipped crate;
    gs::Mipped dirt;
    gs::Mipped swept;
    gs::Mipped pier;
    gs::Mipped lintel;
    gs::Mipped keystone;
    gs::Mipped lamp;
    gs::Mipped crib;
    gs::Mipped rubble;
    gs::Mipped door;
    gs::Mipped dawn;
    gs::Mipped tree;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cler
