// S3 PRESS GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pressgold {

// Five pulls: gold, cream, gold, cream, gold. The last pull is gold.
// Face 7 doubled plus two cream 5s clears 46. Bare faces stay at 31.
constexpr int kPulls = 5;
constexpr int kGoldFace = 7;
constexpr int kCreamFace = 5;
constexpr int kLine = 46;
constexpr int kGolds = 3;
constexpr int kCreams = 2;

inline bool pullGold(int i) { return i == 0 || i == 2 || i == 4; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_WOOD = 5,
    PAL_IRON = 6,
    PAL_INK = 7,
    PAL_MAN = 8,
    PAL_PAPER = 9
};

struct Art {
    gs::Image frame;
    gs::Image platen;
    gs::Image bed;
    gs::Image sheetGold;
    gs::Image sheetCream;
    gs::Image roller;
    gs::Image printer;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pressgold
