// One sheet, played from the hack toward the house. Feet, button at (0, kTee).
#pragma once

#include <cmath>

#include "console/vdp.h"

namespace curlgold {

constexpr float kPx = 14.f;
constexpr int kIceTop = 14;
constexpr int kIceH = 198;
constexpr float kViewFt = float(kIceH) / kPx;

constexpr float kHalfW = 7.85f;
constexpr float kImgTop = 112.f;
constexpr float kImgBot = 16.f;

constexpr float kTee = 100.f;
constexpr float kRelease = 26.f;
constexpr float kHog = 68.f;
constexpr float kBack = 106.f;
constexpr float kSide = 6.7f;
constexpr float kStoneR = 0.48f;
constexpr float kGold = 2.f;    // 4-foot. A stone whose centre lies here counts two.
constexpr float kEight = 4.f;
constexpr float kHouse = 6.f;   // 12-foot. The rest of the house counts one.

constexpr float kA0 = 27.f;
constexpr float kSweepCut = 0.18f;
constexpr float kCurl = 2.15f;
constexpr float kCurlSweep = 0.5f;
constexpr float kStop = 0.18f;
constexpr float kRestitution = 0.74f;
constexpr float kD0 = 42.f;
constexpr float kDSpan = 52.f;

constexpr float kDt = 1.f / 180.f;
constexpr int kSubs = 3;
constexpr float kFrameDt = 1.f / 60.f;
constexpr int kMaxSlide = 60 * 6;
constexpr int kSimSteps = kMaxSlide * kSubs;
constexpr int kRocks = 8;

inline int rinkW() { return int(std::lround(kHalfW * 2.f * kPx)); }
inline int rinkH() { return int(std::lround((kImgTop - kImgBot) * kPx)); }
inline int iceX() { return (gs::SCREEN_W - rinkW()) / 2; }
inline float camMin() { return kImgBot + kViewFt; }
inline float camMax() { return kImgTop; }
inline float camFor(float focus, float bias) {
    float t = focus + kViewFt * bias;
    if (t < camMin()) t = camMin();
    if (t > camMax()) t = camMax();
    return t;
}
inline float screenX(float wx) { return float(iceX()) + (wx + kHalfW) * kPx; }
inline float screenY(float wy, float camTop) { return float(kIceTop) + (camTop - wy) * kPx; }
inline float buttonDist(float x, float y) { return std::hypot(x, y - kTee); }
inline float speedForPower(float p) {
    if (p < 0.f) p = 0.f;
    if (p > 1.f) p = 1.f;
    return std::sqrt(2.f * kA0 * (kD0 + kDSpan * p));
}

struct Shot {
    float aim = 0;
    float power = 0.62f;
    int curl = 1;
    float sweep = 0;
};

struct Rock {
    float x = 0, y = 0, vx = 0, vy = 0;
    int side = 0;
    int curl = 1;
    bool moving = false;
    bool dead = false;
};

struct Sheet {
    Rock r[kRocks]{};
    int n = 0;
};

}  // namespace curlgold
