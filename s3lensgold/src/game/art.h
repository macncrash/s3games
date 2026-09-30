// S3 LENS GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lensgold {

constexpr int kPlates = 6;
constexpr int kGoldFace = 5;
constexpr int kCreamFace = 3;
constexpr int kLine = 42;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Plate order: gold, cream, gold, cream, gold, gold. The last plate is gold.
inline bool plateGold(int i) { return i == 0 || i == 2 || i == 4 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_BRASS = 5,
    PAL_GLASS = 6,
    PAL_BENCH = 7,
    PAL_SPARK = 8
};

struct Art {
    gs::Image lens;
    gs::Image plateGold;
    gs::Image plateCream;
    gs::Image bench;
    gs::Image spark;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lensgold
