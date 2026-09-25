// S3 THERM pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace therm {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_BAG = 4,
    PAL_BASKET = 5,
    PAL_FLAME = 6,
    PAL_TREE = 7,
    PAL_MARK = 8,
    PAL_CLOUD = 10,
    PAL_FLAG = 11,
    PAL_SUN = 12,
    PAL_BIRD = 13,
    PAL_TREE2 = 14,
};

constexpr int kBagBmpW = 60;
constexpr int kBagBmpH = 84;
constexpr int kBasketBmpW = 36;
constexpr int kBasketBmpH = 28;
constexpr int kTreeBmpW = 48;
constexpr int kTreeBmpH = 80;
constexpr int kFlagBmpW = 24;
constexpr int kFlagBmpH = 92;

struct Art {
    gs::Mipped bag;
    gs::Mipped basket;
    gs::Mipped flame[3];
    gs::Mipped tree;
    gs::Mipped mark;
    gs::Mipped flag[2];
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped bird[2];
    gs::Mipped shadow;
    gs::Mipped therm;
    gs::Mipped oneEnv;
    gs::Mipped theMark;
    gs::Mipped theTrees;
    gs::Mipped tooHot;
    gs::Mipped tooHard;
    gs::Mipped missed;
    gs::Mipped drifted;
    gs::Mipped theDay;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace therm
