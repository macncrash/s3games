// Pictures drawn at boot. Face values live on the tower.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace strikergold {

constexpr float kTowerX = 204.f;
constexpr float kTowerTop = 22.f;
constexpr float kTowerH = 168.f;
constexpr int kTowerW = 56;
constexpr int kTowerPixH = 160;

constexpr float kSlotTop = 16.f;
constexpr float kSlotBot = 142.f;
constexpr int kMarkN = 4;
constexpr int kGold = 3;
// Height up the slot, 0 at the pad, 1 at the bell.
constexpr float kMarkH[kMarkN] = {0.18f, 0.40f, 0.62f, 0.90f};
// Face points. Only the gold face is doubled when it is scored.
constexpr int kFace[kMarkN] = {10, 20, 30, 40};
constexpr int kLine = 80;
constexpr float kGoldAt = 0.86f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WOOD = 4,
    PAL_BRASS = 5,
    PAL_NIGHT = 6,
    PAL_MAN = 7,
    PAL_CREAM = 8,
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
    gs::Mipped flag;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

inline float slotScreenY(float h) {
    float sc = kTowerH / float(kTowerPixH);
    float top = kTowerTop + kSlotTop * sc;
    float bot = kTowerTop + kSlotBot * sc;
    return bot - h * (bot - top);
}

inline int scored(int mark) {
    if (mark < 0 || mark >= kMarkN) return 0;
    return mark == kGold ? kFace[mark] * 2 : kFace[mark];
}

}  // namespace strikergold
