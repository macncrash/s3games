// S3 CLOCKMARK pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace clockmark {

constexpr int kCx = 160;
constexpr int kCy = 118;
constexpr int kPivot = 40;
constexpr int kHours = 12;
constexpr int kMark = 4;  // gold pip, four o'clock

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_DIAL = 4,
    PAL_HAND = 5,
    PAL_LIT = 6,
    PAL_COIN = 7,
    PAL_TOWER = 8,
    PAL_SKY = 9
};

struct Art {
    gs::Image hand[kHours];
    gs::Image dial;
    gs::Image coin;
    gs::Image bell;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clockmark
