// S3 CURL SEVEN sheet. Pictures are drawn at boot. No asset files.
// The back line is tangent to the house. A stone touching it is out.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace curlseven {

constexpr float kPx = 12.f;
constexpr int kRinkW = 144;
constexpr int kRinkCol = 11;
constexpr int kRinkX = kRinkCol * 8;
constexpr float kHalf = 6.f;
constexpr float kBoard = 0.75f;
constexpr float kSide = kHalf - kBoard;
constexpr float kNear = 2.f;
constexpr float kFar = kNear + float(gs::SCREEN_H) / kPx;
constexpr float kRelease = 4.4f;
constexpr float kHog = 7.7f;
constexpr float kTee = 15.15f;
constexpr float kHouse = 4.1f;
constexpr float kBack = kTee + kHouse;
constexpr float kStoneR = 0.48f;
constexpr float kBite = kHouse + kStoneR;

static_assert(kRinkW % 8 == 0, "rink width is whole tiles");
static_assert(gs::SCREEN_H % 8 == 0, "rink height is whole tiles");
static_assert((gs::SCREEN_W - kRinkW) / 2 == kRinkX, "rink sits on the tile grid");
static_assert(kRinkW == int(kHalf * 2.f * kPx), "feet match the bitmap");

enum Pal {
    PAL_ICE = 0,
    PAL_RED = 1,
    PAL_YEL = 2,
    PAL_GHOST = 3,
    PAL_DIM = 4,
    PAL_GOLD = 5,
    PAL_INK = 6,
    PAL_TITLE = 7,
    PAL_GREEN = 8,
    PAL_ALERT = 9,
    PAL_WIN = 10,
    PAL_SKIP = 11,
    PAL_BROOM = 12,
    PAL_PUFF = 13,
    PAL_AIM = 14,
    PAL_WHITE = 15
};

inline float worldX(int px) { return -kHalf + (float(px) + 0.5f) / float(kRinkW) * (kHalf * 2.f); }
inline float worldY(int py) { return kFar - (float(py) + 0.5f) / float(gs::SCREEN_H) * (kFar - kNear); }
inline float screenX(float wx) { return float(kRinkX) + (wx + kHalf) / (kHalf * 2.f) * float(kRinkW); }
inline float screenY(float wy) { return (kFar - wy) / (kFar - kNear) * float(gs::SCREEN_H); }
inline float buttonDist(float x, float y) { return std::hypot(x, y - kTee); }

struct Art {
    gs::Mipped stone;
    gs::Image pip;
    gs::Image broom;
    gs::Image puff;
    gs::Image dot;
    gs::Image shadow;
    gs::Image skip;
    gs::Image curl;
    gs::Image seven;
    gs::Image big;
    gs::Image shorty;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace curlseven
