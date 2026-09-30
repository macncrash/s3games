// S3 BELL GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bellgold {

constexpr int kRings = 6;
constexpr int kGoldFace = 5;
constexpr int kCreamFace = 3;
constexpr int kLine = 42;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Ring order: gold, cream, gold, cream, gold, gold. The last pull is gold.
inline bool ringGold(int i) { return i == 0 || i == 2 || i == 4 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_BRONZE = 5,
    PAL_ROPE = 6,
    PAL_RINGER = 7,
    PAL_STONE = 8
};

struct Art {
    gs::Image tower;
    gs::Image bell;
    gs::Image clapper;
    gs::Image rope;
    gs::Image ringer;
    gs::Image lip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bellgold
