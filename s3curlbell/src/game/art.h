// S3 CURLBELL pictures and the sheet they share with the stones.
// Drawn at boot. No asset files.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace curlbell {

constexpr float kPx = 12.f;
constexpr float kNear = 1.8f;
constexpr float kFar = kNear + float(gs::SCREEN_H) / kPx;
constexpr float kHack = 3.40f;
constexpr float kHog = 7.20f;
constexpr float kTee = 14.55f;
constexpr float kHouse = 3.95f;
constexpr float kBack = 18.50f;
constexpr float kSide = 6.05f;
constexpr float kBoard = 0.70f;
constexpr float kStoneR = 0.48f;
constexpr float kBellR = 0.55f;
constexpr float kGuardX = 4.15f;
constexpr float kGuardY = 10.40f;
constexpr int kMaxThrow = 3;
constexpr int kMaxRock = 4;

enum Pal {
    PAL_ICE = 0,
    PAL_RED = 1,
    PAL_YEL = 2,
    PAL_GHOST = 3,
    PAL_BELL = 4,
    PAL_INK = 5,
    PAL_TITLE = 6,
    PAL_GREEN = 7,
    PAL_ALERT = 8,
    PAL_BROOM = 9,
    PAL_PUFF = 10,
    PAL_WIN = 11,
    PAL_AIM = 12,
    PAL_DIM = 13,
    PAL_SKIP = 14
};

inline float sheetL() { return -(kSide + kBoard); }
inline float sheetR() { return kSide + kBoard; }
inline int iceW() { return int(std::lround((sheetR() - sheetL()) * kPx)); }
inline int iceX() { return (gs::SCREEN_W - iceW()) / 2; }

inline float worldX(int px) {
    return sheetL() + (float(px - iceX()) + 0.5f) / float(iceW()) * (sheetR() - sheetL());
}
inline float worldY(int py) {
    return kFar - (float(py) + 0.5f) / float(gs::SCREEN_H) * (kFar - kNear);
}
inline float screenX(float wx) {
    return float(iceX()) + (wx - sheetL()) / (sheetR() - sheetL()) * float(iceW());
}
inline float screenY(float wy) { return (kFar - wy) / (kFar - kNear) * float(gs::SCREEN_H); }

struct Art {
    gs::Mipped stone;
    gs::Image bell;
    gs::Image clapper;
    gs::Image skip;
    gs::Image broom;
    gs::Image puff;
    gs::Image dot;
    gs::Image shadow;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curlbell
