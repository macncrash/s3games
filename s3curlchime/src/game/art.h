// S3 CURLCHIME pictures and the sheet. Drawn at boot. No asset files.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace curlchime {

constexpr float kNear = 0.35f;
constexpr float kFar = 17.15f;
constexpr float kHack = 2.10f;
constexpr float kHog = 6.40f;
constexpr float kTee = 12.70f;
constexpr float kBack = 16.05f;
constexpr float kSide = 4.65f;
constexpr float kBoard = 0.58f;
constexpr float kHouse = 2.80f;
constexpr float kStoneR = 0.42f;
constexpr float kButton = 0.50f;
constexpr float kSheetL = -(kSide + kBoard);
constexpr float kSheetR = kSide + kBoard;
constexpr int kIceX = 84;
constexpr int kIceW = 152;
constexpr int kViewTop = 16;
constexpr int kViewH = 190;
constexpr int kDial = 46;

enum Pal {
    PAL_ICE = 0,
    PAL_RED = 1,
    PAL_YEL = 2,
    PAL_GHOST = 3,
    PAL_INK = 4,
    PAL_GOLD = 5,
    PAL_GREEN = 6,
    PAL_ALERT = 7,
    PAL_DIM = 8,
    PAL_BROOM = 9,
    PAL_SKIP = 10,
    PAL_FACE = 11,
    PAL_HAND = 12,
    PAL_BELL = 13,
    PAL_PUFF = 14,
    PAL_WIN = 15
};

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

struct Art {
    int font[96] = {};
    gs::Mipped stone;
    gs::Mipped skip[2];
    gs::Image broom;
    gs::Image puff;
    gs::Image dot;
    gs::Image shadow;
    gs::Image face;
    gs::Image ring;
    gs::Image cap;
    gs::Image hand[3][60];
    gs::Image bell;
    gs::Image clapper;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curlchime
