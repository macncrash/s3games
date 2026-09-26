// S3 CURLMARK pictures and the sheet they share with the rocks.
// Drawn at boot. No asset files.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace curlmark {

constexpr float kPx = 12.f;
constexpr float kNear = 20.5f;
constexpr float kFar = kNear + float(gs::SCREEN_H) / kPx;
constexpr float kReleaseX = 0.f;
constexpr float kReleaseY = 22.35f;
constexpr float kHog = 25.9f;
constexpr float kTee = 32.55f;
constexpr float kHouse = 5.55f;
constexpr float kBack = 38.45f;
constexpr float kSide = 6.75f;
constexpr float kBoard = 0.62f;
constexpr float kMarkX = 1.55f;
constexpr float kMarkY = kTee;
constexpr float kGuardX = 2.05f;
constexpr float kGuardY = 30.55f;
constexpr float kStoneR = 0.50f;

constexpr float kA = 15.f;
constexpr float kC = 6.2f;
constexpr float kStop = 0.15f;
constexpr float kBite = 0.40f;
constexpr float kSweepDrag = 0.18f;
constexpr float kSweepCurl = 0.62f;
constexpr int kMaxThrow = 4;
constexpr int kMaxRock = 8;

enum Pal {
    PAL_ICE = 0,
    PAL_RED = 1,
    PAL_YEL = 2,
    PAL_GOLD = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_GREEN = 6,
    PAL_ALERT = 7,
    PAL_GHOST = 8,
    PAL_BROOM = 9,
    PAL_PUFF = 10,
    PAL_WIN = 11,
    PAL_AIM = 12,
    PAL_DIM = 13
};

inline float sheetL() { return -(kSide + kBoard); }
inline float sheetR() { return kSide + kBoard; }
inline int rinkW() { return int(std::lround((sheetR() - sheetL()) * kPx)); }
inline int rinkH() { return gs::SCREEN_H; }
inline int rinkX() { return (gs::SCREEN_W - rinkW()) / 2; }

inline float worldX(int px) {
    return sheetL() + (float(px) + 0.5f) / float(rinkW()) * (sheetR() - sheetL());
}
inline float worldY(int py) {
    return kFar - (float(py) + 0.5f) / float(rinkH()) * (kFar - kNear);
}
inline float screenX(float wx) {
    return float(rinkX()) + (wx - sheetL()) / (sheetR() - sheetL()) * float(rinkW());
}
inline float screenY(float wy) { return (kFar - wy) / (kFar - kNear) * float(rinkH()); }

struct Art {
    gs::Image rink;
    gs::Mipped stone;
    gs::Image mark;
    gs::Image broom;
    gs::Image puff;
    gs::Image dot;
    gs::Image shadow;
    gs::Image logo;
    gs::Image fin;
    gs::Image open;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curlmark
