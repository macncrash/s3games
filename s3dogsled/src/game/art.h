// S3 DOGSLED pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sled {

// Trail arch proportions. Collision uses the same fractions as the bitmap.
constexpr int GATE_W = 96;
constexpr int GATE_POST_M = 8;
constexpr int GATE_POST_W = 10;
constexpr float GATE_HALF = GATE_W * 0.5f;
constexpr float GATE_POST_FRAC = (GATE_HALF - (GATE_POST_M + GATE_POST_W * 0.5f)) / GATE_HALF;
constexpr float GATE_OPEN_FRAC = (GATE_HALF - (GATE_POST_M + GATE_POST_W)) / GATE_HALF;
constexpr float GATE_POST_HALF_FRAC = (GATE_POST_W * 0.5f) / GATE_HALF;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_DOG = 4,
    PAL_SLED = 5,
    PAL_TREE = 6,
    PAL_GATE = 7,
    PAL_FX = 8,
    PAL_CABIN = 9,
    PAL_AURORA = 10,
    PAL_SKY = 11,
    PAL_SNOW = 12,
    PAL_ICE = 13,
    PAL_HARE = 14,
    PAL_LEAD = 15
};

struct Art {
    gs::Mipped dog[3];
    gs::Mipped sled[3];
    gs::Mipped spruce;
    gs::Mipped rock;
    gs::Mipped gate;
    gs::Mipped lantern;
    gs::Mipped cabin;
    gs::Mipped hare;
    gs::Mipped moon;
    gs::Mipped aurora;
    gs::Mipped puff;
    gs::Mipped flake;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int starTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sled
