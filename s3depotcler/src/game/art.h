// S3 DEPOT CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace dcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BRICK = 4,
    PAL_CLERK = 5,
    PAL_SACK = 6,
    PAL_WOOD = 7,
    PAL_STEEL = 8,
    PAL_FX = 9,
    PAL_NIGHT = 10,
    PAL_CAR = 11,
    PAL_DOCK = 12
};

struct Art {
    gs::Mipped clerk[2];
    gs::Mipped dolly;
    gs::Mipped crate;
    gs::Mipped drum;
    gs::Mipped sack;
    gs::Mipped boards;
    gs::Mipped car;
    gs::Mipped cargo;
    gs::Mipped wall;
    gs::Mipped clock;
    gs::Mipped lamp;
    gs::Mipped stack;
    gs::Mipped chimney;
    gs::Mipped rails;
    gs::Mipped truck;
    gs::Mipped steam;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dcler
