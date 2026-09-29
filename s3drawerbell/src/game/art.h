// S3 DRAWERBELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drawerbell {

constexpr int kCoins = 6;
constexpr int kTarget = 40;
constexpr int kMaxDead = 3;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_WOOD = 4,
    PAL_COIN = 5,
    PAL_SLIP = 6,
    PAL_BELL = 7,
    PAL_LIT = 8,
    PAL_CLERK = 9,
    PAL_INK = 10
};

struct Art {
    gs::Image coin[4];
    gs::Image drawer;
    gs::Image bell;
    gs::Image clapper;
    gs::Image clerk;
    gs::Image slip;
    gs::Image mark;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawerbell
