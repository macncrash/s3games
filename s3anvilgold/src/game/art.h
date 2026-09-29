// S3 ANVIL GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace anvilgold {

constexpr int kHeats = 6;
constexpr int kGoldFace = 5;
constexpr int kCreamFace = 3;
constexpr int kLine = 42;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Heat order: gold, cream, gold, cream, gold, gold. The last blow is gold.
inline bool heatGold(int i) { return i == 0 || i == 2 || i == 4 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_IRON = 5,
    PAL_FIRE = 6,
    PAL_SMITH = 7,
    PAL_WOOD = 8
};

struct Art {
    gs::Image anvil;
    gs::Image hammer;
    gs::Image barGold;
    gs::Image barCream;
    gs::Image spark;
    gs::Image smith;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace anvilgold
