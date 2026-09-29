// S3 DRAWER GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drawergold {

constexpr int kLine = 40;
constexpr int kGoldFace = 10;
constexpr int kCreamFace = 8;
constexpr int kGoldNeed = 2;
constexpr int kSlips = 6;

// G files double. C is cream and stays single.
inline char slipKind(int i) { return "GCGCGC"[i]; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_DESK = 5,
    PAL_SLIP = 6,
    PAL_PAPER = 7,
    PAL_BRASS = 8
};

struct Art {
    gs::Image desk;
    gs::Image drawer;
    gs::Image gold;
    gs::Image cream;
    gs::Image knob;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawergold
