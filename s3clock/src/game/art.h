// S3 CLOCK pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace clk {

constexpr int kCx = 160;
constexpr int kCy = 120;
constexpr int kTowerX = 104;
constexpr int kTowerY = 24;
constexpr int kTowerW = 112;
constexpr int kTowerH = 176;
constexpr int kHandN = 60;
constexpr int kHandS = 97;
constexpr int kPivot = 48;
constexpr int kBellY = 58;
constexpr int kRopeX = 210;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_SCENE = 4,
    PAL_WORK = 5,
    PAL_HOUR = 6,
    PAL_HLIT = 7,
    PAL_MIN = 8,
    PAL_MLIT = 9,
    PAL_SEC = 10,
    PAL_SLIT = 11,
    PAL_BELL = 12,
    PAL_MOON = 13,
    PAL_RING = 14,
    PAL_BOB = 15
};

struct Art {
    gs::Image hand[3][kHandN];
    gs::Image cap, bell, rope, moon, star, bob, rod, ring;
    gs::Image glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clk
