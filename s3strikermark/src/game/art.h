// Pictures drawn at boot. The tower marks are the scoring rules.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace strikermark {

// Screen placement of the tower, shared by the painter and the puck.
constexpr float kTowerX = 196.f;
constexpr float kTowerTop = 18.f;
constexpr float kTowerH = 176.f;
constexpr int kTowerW = 52;
constexpr int kTowerPixH = 168;

// Slot in tower pixels. The gold mark is the only finish.
constexpr float kSlotX = 24.f;
constexpr float kSlotTop = 18.f;
constexpr float kSlotBot = 150.f;
constexpr int kMarkN = 5;
constexpr int kGold = 4;
// Height up the slot, 0 at the anvil, 1 at the bell.
constexpr float kMarkH[kMarkN] = {0.14f, 0.32f, 0.50f, 0.68f, 0.94f};
constexpr float kFinishAt = 0.88f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WOOD = 4,
    PAL_BRASS = 5,
    PAL_NIGHT = 6,
    PAL_MAN = 7,
    PAL_FX = 8,
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
    return bot - h * (bot - top);
}

}  // namespace strikermark
