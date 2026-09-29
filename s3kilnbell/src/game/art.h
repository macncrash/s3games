// S3 KILN BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilnbell {

constexpr int kTries = 3;
constexpr int kDieAt = 64;
constexpr int kSweetLo = 34;
constexpr int kSweetHi = 46;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BRICK = 4,
    PAL_CLAY = 5,
    PAL_FIRE = 6,
    PAL_BELL = 7,
    PAL_ASH = 8
};

struct Art {
    gs::Image kiln;
    gs::Image pot;
    gs::Image crack;
    gs::Image flame;
    gs::Image cone;
    gs::Image bar;
    gs::Image bell;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilnbell
