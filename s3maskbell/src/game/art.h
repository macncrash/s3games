// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace maskbell {

constexpr int kCuts = 4;
constexpr int kPhase = 48;
constexpr int kMaxDead = 3;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_FACE = 4,
    PAL_HOLE = 5,
    PAL_BELL = 6,
    PAL_LIT = 7,
    PAL_TICK = 8,
    PAL_BENCH = 9,
    PAL_WOOD = 10
};

struct Art {
    gs::Image mask;
    gs::Image hole;
    gs::Image bell;
    gs::Image tick;
    gs::Image rail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maskbell
