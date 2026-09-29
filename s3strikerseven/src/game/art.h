// Pictures drawn at boot. There are no asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace strikerseven {

constexpr float kTowerX = 198.f;
constexpr float kTowerTop = 16.f;
constexpr float kTowerH = 180.f;
constexpr int kTowerW = 48;
constexpr int kTowerPixH = 160;

constexpr float kSlotTop = 16.f;
constexpr float kSlotBot = 142.f;

// Height up the slot, 0 at the anvil, 1 at the bell.
constexpr float kTickH = 0.22f;
constexpr float kPairH = 0.48f;
constexpr float kBellH = 0.82f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WOOD = 4,
    PAL_BRASS = 5,
    PAL_NIGHT = 6,
    PAL_MAN = 7,
    PAL_THEM = 8,
    PAL_WIN = 9,
    PAL_CROWD = 10
};

struct Art {
    gs::Mipped tower;
    gs::Mipped puck;
    gs::Mipped bell[2];
    gs::Mipped man[3];
    gs::Mipped mallet;
    gs::Mipped lamp;
    gs::Mipped bunt;
    gs::Mipped crowd;
    gs::Image title;
    gs::Image banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

inline float slotScreenY(float h) {
    float sc = kTowerH / float(kTowerPixH);
    float top = kTowerTop + kSlotTop * sc;
    float bot = kTowerTop + kSlotBot * sc;
    if (h < 0.f) h = 0.f;
    if (h > 1.f) h = 1.f;
    return bot - h * (bot - top);
}

}  // namespace strikerseven
