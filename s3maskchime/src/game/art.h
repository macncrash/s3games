// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace maskchime {

constexpr int kCuts = 4;
constexpr int kTries = 3;
constexpr int kPeriod = 40;
constexpr int kFpc = 8;
constexpr int kGraceSec = 16;
constexpr int kHourSec = 12 * 3600;
constexpr int kLeadSec = 72;

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
    PAL_WOOD = 9,
    PAL_CLOCK = 10,
    PAL_INK = 11
};

struct Art {
    gs::Image mask;
    gs::Image hole;
    gs::Image bell;
    gs::Image tick;
    gs::Image rail;
    gs::Image face;
    gs::Image pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maskchime
