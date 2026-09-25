// Sheet geometry shared by the painted ice and the rocks.
// Feet, origin at the hack end. The button is (0, kTee).
#pragma once

#include <cmath>

#include "console/vdp.h"

namespace curl {

constexpr float kPx = 12.f;
constexpr int kIceTop = 18;
constexpr int kIceH = 190;
constexpr float kViewFt = float(kIceH) / kPx;
constexpr float kHalfW = 7.55f;
constexpr float kImgTop = 110.f;
constexpr float kImgBot = 24.f;

constexpr float kTee = 100.f;
constexpr float kRelease = 30.f;
constexpr float kHogLine = 70.f;
constexpr float kBackLine = 106.f;
constexpr float kSideLine = 6.5f;
constexpr float kStoneR = 0.46f;
constexpr float kHouse = 6.f;

// Straight-line run with no curl and no sweep: d = kD0 + kDSpan * power.
constexpr float kD0 = 38.f;
constexpr float kDSpan = 58.f;
constexpr float kA0 = 28.f;
constexpr float kSweepCut = 0.16f;
constexpr float kCurl = 3.4f;
constexpr float kCurlSweep = 0.55f;
constexpr float kStop = 0.22f;
constexpr float kRestitution = 0.8f;

constexpr float kDt = 1.f / 180.f;
constexpr int kSubs = 3;
constexpr float kFrameDt = 1.f / 60.f;
constexpr int kSimSteps = 180 * 5;

constexpr int kEnds = 4;
constexpr int kPerSide = 4;

inline int rinkPixelW() { return int(std::lround(kHalfW * 2.f * kPx)); }
inline int rinkPixelH() { return int(std::lround((kImgTop - kImgBot) * kPx)); }
inline int iceX() { return (gs::SCREEN_W - rinkPixelW()) / 2; }
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

inline bool inPlay(float x, float y) {
    if (std::fabs(x) > kSideLine) return false;
    if (y < kHogLine + kStoneR) return false;
    if (y > kBackLine + kStoneR) return false;
    return true;
}

inline float buttonDist(float x, float y) { return std::hypot(x, y - kTee); }

inline float speedForPower(float p) {
    if (p < 0.f) p = 0.f;
    if (p > 1.f) p = 1.f;
    return std::sqrt(2.f * kA0 * (kD0 + kDSpan * p));
}

struct Shot {
    float aim = 0;
    float power = 0.55f;
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
    Rock r[8]{};
    int n = 0;
};

}  // namespace curl
