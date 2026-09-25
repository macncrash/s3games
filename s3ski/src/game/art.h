// S3 SKI sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ski {

// Pole bitmap is kPoleW wide. The shaft column center is kPoleShaft so a
// horizontal flip parks the flag on the other side of the same shaft.
constexpr int kPoleW = 28;
constexpr float kPoleShaft = 7.5f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SKIER = 4,
    PAL_GATE_R = 5,
    PAL_GATE_B = 6,
    PAL_TREE = 7,
    PAL_ROCK = 8,
    PAL_MAN = 9,
    PAL_FX = 10,
    PAL_BANNER = 11,
    PAL_ROAD = 12,
    PAL_MOUNT = 13,
    PAL_SUN = 14
};

struct Art {
    gs::Mipped skier;
    gs::Mipped carve;
    gs::Mipped fall;
    gs::Mipped pole;
    gs::Mipped tree;
    gs::Mipped rock;
    gs::Mipped man;
    gs::Mipped mount;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped banner;
    gs::Mipped puff;
    gs::Mipped shadow;
    gs::Mipped logo;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ski
