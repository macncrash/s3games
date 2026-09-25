// S3 PARADE sprites and tiles. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace parade {

enum Pal {
    PAL_WHITE = 0,
    PAL_MAJOR = 1,
    PAL_HORSE = 2,
    PAL_WAGON = 3,
    PAL_DRUM = 4,
    PAL_BATON = 5,
    PAL_SQUARE = 6,
    PAL_CROWD = 7,
    PAL_STREET = 8,
    PAL_CONFETTI = 9,
    PAL_PENNANT = 10,
    PAL_FX = 11,
    PAL_GOLD = 13,
    PAL_RED = 14,
    PAL_GREEN = 15
};

struct Art {
    gs::Mipped major[2];
    gs::Mipped horse[2];
    gs::Mipped wagon[2];
    gs::Mipped drum[2];
    gs::Mipped baton[2];
    gs::Mipped square;
    gs::Mipped ring;
    gs::Mipped person[3];
    gs::Mipped pennant;
    gs::Mipped confetti;
    gs::Mipped shadow;
    gs::Mipped chevron;
    gs::Mipped puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

// rows: world-y of each crossing, used to paint the street guides.
void buildArt(gs::VDP& vdp, Art& art, const float* rows, int nrows);

}  // namespace parade
