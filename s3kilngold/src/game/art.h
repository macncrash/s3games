// S3 KILN GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilngold {

constexpr int kFires = 6;
constexpr int kGoldFace = 5;
constexpr int kCreamFace = 3;
constexpr int kLine = 42;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Fire order: gold, cream, gold, cream, gold, gold. The last pot is gold.
inline bool fireGold(int i) { return i == 0 || i == 2 || i == 4 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_BRICK = 5,
    PAL_FIRE = 6,
    PAL_CLAY = 7,
    PAL_ASH = 8
};

struct Art {
    gs::Image kiln;
    gs::Image potGold;
    gs::Image potCream;
    gs::Image flame;
    gs::Image shelf;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilngold
