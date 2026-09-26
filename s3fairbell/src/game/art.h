// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fairbell {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_BLUE = 3,
    PAL_KID = 4,
    PAL_WOOD = 5,
    PAL_BRASS = 6,
    PAL_NIGHT = 7,
    PAL_PINK = 8,
    PAL_CREAM = 9,
    PAL_BULB = 10,
    PAL_LOGO = 11,
    PAL_GROUND = 12,
    PAL_BAD = 13,
    PAL_GATE = 14,
    PAL_AWN = 15
};

// Tower image rows. The slot is open sky, then a red cap, then the gold plate.
// Screen placement of that image is shared so the puck and the paint agree.
constexpr int kTowerImgW = 40;
constexpr int kTowerImgH = 110;
constexpr int kGoldTop = 16;
constexpr int kGoldBot = 26;
constexpr int kCapTop = 4;
constexpr int kCapBot = 15;
constexpr float kSlotBase = 100.f;
constexpr float kSlotCap = 6.f;
constexpr float kTowerFeet = 198.f;
constexpr float kTowerDrawH = 150.f;
constexpr float kTowerX = 188.f;

struct Art {
    int font[96] = {};
    gs::Mipped kid[2];
    gs::Mipped hammer[2];
    gs::Mipped bell, puck, tower, awning, gate;
    gs::Mipped bulb, pennant, balloon, moon, star, burst, shadow;
    gs::Mipped car, hub, stand;
    gs::Image logo;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fairbell
