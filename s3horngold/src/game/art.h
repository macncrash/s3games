// S3 HORN GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace horngold {

// Six notes of a short post-horn call. Four gold, two cream. Last note is gold.
constexpr int kCalls = 6;
constexpr int kGoldFace = 8;
constexpr int kCreamFace = 5;
constexpr int kLine = 70;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Order: gold, cream, gold, gold, cream, gold.
inline bool callGold(int i) { return i == 0 || i == 2 || i == 3 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_BRASS = 5,
    PAL_COAT = 6,
    PAL_YARD = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Image player;
    gs::Image horn;
    gs::Image bell;
    gs::Image breath;
    gs::Image note;
    gs::Image coach;
    gs::Image lamp;
    gs::Image rail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace horngold
