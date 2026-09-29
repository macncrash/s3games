// S3 CLOCK GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace clockgold {

constexpr int kCx = 160;
constexpr int kCy = 122;
constexpr int kPivot = 46;
constexpr int kHours = 12;
constexpr int kLine = 60;
constexpr int kGoldFace = 10;
constexpr int kCreamFace = 9;
constexpr int kGoldNeed = 3;

// Hour index 0 is twelve. Golds are 2, 6 and 10. Cream is 4, 8 and 12.
inline bool goldHour(int h) { return h == 2 || h == 6 || h == 10; }
inline bool creamHour(int h) { return h == 0 || h == 4 || h == 8; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_DIAL = 4,
    PAL_HAND = 5,
    PAL_LIT = 6,
    PAL_CREAM = 7,
    PAL_TOWER = 8,
    PAL_BELL = 9,
    PAL_SEC = 10
};

struct Art {
    gs::Image hour[kHours];
    gs::Image minute;
    gs::Image second[kHours];
    gs::Image dial;
    gs::Image pipGold;
    gs::Image pipCream;
    gs::Image bell;
    gs::Image rope;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clockgold
