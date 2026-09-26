// S3 SKIFF LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skifflock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOAT = 4,
    PAL_GATE = 5,
    PAL_STONE = 6,
    PAL_BANK = 7,
    PAL_FX = 8,
    PAL_KEEP = 9,
    PAL_HOUSE = 10,
    PAL_PORT = 11,
    PAL_STBD = 12,
    PAL_BIRD = 13
};

constexpr int YAWS = 16;
constexpr int GATE_DIRS = 48;
constexpr int SKIFF_PX = 64;  // nose-to-stern pixels in the skiff bitmap
constexpr int LEAF_PX = 56;   // timber length in the unrotated leaf

struct Art {
    gs::Mipped skiff[YAWS];
    gs::Mipped leaf[GATE_DIRS];
    gs::Mipped grass, field, stone, reed, tree;
    gs::Mipped cottage, sign, keeper, post, buoy;
    gs::Mipped heron, gull[2];
    gs::Mipped ripple, foam, blob;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skifflock
