// S3 ROCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace rock {

// Opaque rock radius as a fraction of the bitmap height. The playfield uses
// the same fraction so the hit circle matches the picture.
constexpr float ROCK_FILL = 0.40f;

enum Pal {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_ROCK = 2,
    PAL_WARM = 3,
    PAL_MOSS = 4,
    PAL_FX = 5,
    PAL_GOLD = 6,
    PAL_BAD = 7,
    PAL_WATER = 10,
    PAL_BANK = 11
};

struct Art {
    gs::Mipped boat[3];     // lean left, level, lean right
    gs::Mipped rock[3][3];  // pebble, stone, boulder, three shapes each
    gs::Mipped splash[2];
    gs::Mipped foam;
    gs::Mipped shade;
    gs::Mipped crack;
    gs::Mipped rib;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rock
