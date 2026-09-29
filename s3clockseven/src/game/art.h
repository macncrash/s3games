// Pictures for the clock. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace c7 {

constexpr int kCx = 160;
constexpr int kCy = 116;
constexpr int kHandN = 60;
constexpr int kPivot = 28;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_FACE = 4,
    PAL_HOUR = 5,
    PAL_MIN = 6,
    PAL_SEC = 7,
    PAL_LIT = 8,
    PAL_BELL = 9,
    PAL_YOU = 10,
    PAL_THEM = 11
};

struct Art {
    gs::Image hand[3][kHandN];
    gs::Image face, cap, bell, rope, lampOn, lampOff, star;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace c7
