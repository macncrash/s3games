// S3 LENS SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lensseven {

constexpr int kSeven = 7;
constexpr int kGoldFace = 2;
constexpr int kCreamFace = 1;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_BRASS = 5,
    PAL_GLASS = 6,
    PAL_RAIL = 7,
    PAL_FLARE = 8
};

struct Art {
    gs::Image lens;
    gs::Image plateGold;
    gs::Image plateCream;
    gs::Image rail;
    gs::Image flare;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lensseven
