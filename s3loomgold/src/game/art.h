// S3 LOOM GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace loomgold {

constexpr int kPicks = 6;
constexpr int kGoldFace = 5;
constexpr int kCreamFace = 3;
constexpr int kLine = 42;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Pick order: gold, cream, gold, cream, gold, gold. The last weft is gold.
inline bool pickGold(int i) { return i == 0 || i == 2 || i == 4 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_WOOD = 5,
    PAL_WARP = 6,
    PAL_SHUTTLE = 7
};

struct Art {
    gs::Image post;
    gs::Image beam;
    gs::Image warp;
    gs::Image weftGold;
    gs::Image weftCream;
    gs::Image shuttle;
    gs::Image reed;
    gs::Image bobbin;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace loomgold
