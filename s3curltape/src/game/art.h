// S3 CURLTAPE pictures and the sheet the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants three lies: BUTTON 12, GUARD 6, BITE 4.
// FOUR, SHORT and RING pay those same amounts and stay out of the drawer.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace curltape {

constexpr float kHack = 1.10f;
constexpr float kHog = 5.15f;
constexpr float kTee = 10.70f;
constexpr float kBack = 12.65f;
constexpr float kSide = 2.30f;
constexpr float kHouse = 1.82f;
constexpr float kEight = kHouse * (2.f / 3.f);
constexpr float kFour = kHouse / 3.f;
constexpr float kButton = 0.22f;
constexpr float kStoneR = 0.22f;
constexpr float kGuardLane = 0.50f;
constexpr float kBiteFrac = 0.78f;

constexpr float kNear = 0.20f;
constexpr float kFar = 13.20f;
constexpr int kIceX = 96;
constexpr int kIceW = 120;
constexpr int kViewTop = 16;
constexpr int kViewH = 192;
constexpr float kBoard = 0.62f;
constexpr float kSheetL = -(kSide + kBoard);
constexpr float kSheetR = kSide + kBoard;

enum Pal {
    PAL_ICE = 0,
    PAL_ROCK = 1,
    PAL_GHOST = 2,
    PAL_SKIP = 3,
    PAL_BROOM = 4,
    PAL_SLIP = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_GREEN = 8,
    PAL_ALERT = 9,
    PAL_DIM = 10,
    PAL_WIN = 11,
    PAL_PUFF = 12,
    PAL_AIM = 13
};

enum class Lie : int {
    Miss = 0,
    Button,
    Guard,
    Bite,
    Four,
    Short,
    Ring,
    House,
    Hog,
    Wide,
    Through,
    Back
};

inline float guardClearD() { return kHouse + kStoneR + 0.04f; }

inline float guardY() {
    float yMin = kHog + 0.12f;
    float yMax = kTee - guardClearD();
    return (yMin + yMax) * 0.5f;
}

inline float biteY() { return kTee - (kHouse + 0.36f * kStoneR); }

inline float screenX(float wx) {
    return float(kIceX) + (wx - kSheetL) / (kSheetR - kSheetL) * float(kIceW);
}
inline float screenY(float wy) {
    return float(kViewTop) + (kFar - wy) / (kFar - kNear) * float(kViewH);
}
inline float worldX(int px) {
    return kSheetL + (float(px - kIceX) + 0.5f) / float(kIceW) * (kSheetR - kSheetL);
}
inline float worldY(int py) {
    return kFar - (float(py - kViewTop) + 0.5f) / float(kViewH) * (kFar - kNear);
}

inline float slotX(int i) {
    (void)i;
    return 40.f;
}
inline float slotY(int i) { return 58.f + float(i) * 38.f; }

inline const char* tapeName(int i) {
    if (i == 0) return "BUTTON";
    if (i == 1) return "GUARD";
    if (i == 2) return "BITE";
    return "";
}
inline int tapePay(int i) {
    if (i == 0) return 12;
    if (i == 1) return 6;
    if (i == 2) return 4;
    return 0;
}

inline const char* lieName(Lie L) {
    switch (L) {
    case Lie::Button: return "BUTTON";
    case Lie::Guard: return "GUARD";
    case Lie::Bite: return "BITE";
    case Lie::Four: return "FOUR";
    case Lie::Short: return "SHORT";
    case Lie::Ring: return "RING";
    case Lie::House: return "HOUSE";
    case Lie::Hog: return "HOG";
    case Lie::Wide: return "WIDE";
    case Lie::Through: return "THROUGH";
    case Lie::Back: return "BACK";
    case Lie::Miss: return "MISS";
    }
    return "MISS";
}

inline int liePay(Lie L) {
    if (L == Lie::Button || L == Lie::Four) return 12;
    if (L == Lie::Guard || L == Lie::Short) return 6;
    if (L == Lie::Bite || L == Lie::Ring) return 4;
    return 0;
}

inline int tapeOf(Lie L) {
    if (L == Lie::Button) return 0;
    if (L == Lie::Guard) return 1;
    if (L == Lie::Bite) return 2;
    return -1;
}

inline int twinOf(Lie L) {
    if (L == Lie::Four) return 0;
    if (L == Lie::Short) return 1;
    if (L == Lie::Ring) return 2;
    return -1;
}

// A resting stone. Lookalikes share a pay with a tape line and are not that line.
inline Lie classify(float x, float y) {
    if (std::fabs(x) > kSide - kStoneR) return Lie::Wide;
    if (y > kBack) return Lie::Through;
    float d = std::hypot(x, y - kTee);
    bool over = y >= kHog;
    if (over && d <= kButton) return Lie::Button;
    if (over && d <= kFour) return Lie::Four;
    if (over && d <= kEight) return Lie::House;
    float biteOut = kHouse + kBiteFrac * kStoneR;
    if (over && y < kTee && d > kHouse && d <= biteOut && std::fabs(x) <= kHouse + kStoneR) return Lie::Bite;
    if (over && d > kEight && d <= kHouse - 0.03f) return Lie::Ring;
    if (y >= kTee && d > kHouse) return Lie::Back;
    if (over && std::fabs(x) <= kGuardLane && d >= guardClearD()) return Lie::Guard;
    if (!over && std::fabs(x) <= kGuardLane && y >= kHog - 1.20f && y >= kHack + 0.70f) return Lie::Short;
    if (!over) return Lie::Hog;
    return Lie::Miss;
}

struct Art {
    int font[96] = {};
    gs::Mipped rock;
    gs::Mipped skip[2];
    gs::Image broom;
    gs::Image slip;
    gs::Image arrow;
    gs::Image shadow;
    gs::Image dot;
    gs::Image puff;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curltape
