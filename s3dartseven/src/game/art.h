// S3 DART SEVEN pictures. Drawn at boot. No asset files.
// The radii below are the scoring rules as well as the paint.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace dartseven {

constexpr int kRace = 7;
constexpr int kBoard = 160;
constexpr float kBmpC = 80.f;
constexpr int kBoardX = 80;
constexpr int kBoardY = 36;
constexpr float kCx = kBoardX + kBmpC;
constexpr float kCy = kBoardY + kBmpC;

constexpr float kBullIn = 4.5f;
constexpr float kBullOut = 11.f;
constexpr float kTripIn = 34.f;
constexpr float kTripOut = 44.f;
constexpr float kDoubIn = 58.f;
constexpr float kDoubOut = 66.f;
constexpr float kWoodIn = 69.f;
constexpr float kNumR = 73.f;
constexpr float kRim = 78.f;

constexpr float kRTrip = (kTripIn + kTripOut) * 0.5f;
constexpr float kRSing = (kTripOut + kDoubIn) * 0.5f;
constexpr float kRDoub = (kDoubIn + kDoubOut) * 0.5f;
constexpr float kRInner = (kBullOut + kTripIn) * 0.5f;
constexpr float kR25 = (kBullIn + kBullOut) * 0.5f;

// Clockwise from the top. 20 is the fast bed.
constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};
constexpr int kFastSeg = 0;

constexpr float kLampYou = 30.f;
constexpr float kLampHouse = 290.f;
constexpr float kLampY0 = 58.f;
constexpr float kLampStep = 16.f;
constexpr float kMeterY = 24.f;

enum Pal {
    PAL_BOARD = 0,
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_GREEN = 3,
    PAL_RED = 4,
    PAL_AIM = 5,
    PAL_YOU = 6,
    PAL_HOUSE = 7,
    PAL_TITLE = 8,
    PAL_WIN = 9,
    PAL_LOSE = 10,
    PAL_PIPY = 11,
    PAL_PIPH = 12,
    PAL_PIPE = 13,
    PAL_METER = 14
};

inline int sectorAt(float dx, float dy) {
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    int s = int(std::floor((deg + 9.f) / 18.f));
    if (s < 0 || s >= 20) s = 0;
    return s;
}

struct Art {
    gs::Image board;
    gs::Image rail;
    gs::Image title;
    gs::Image win;
    gs::Image lose;
    gs::Mipped dart;
    gs::Image cross;
    gs::Image dot;
    gs::Image pip;
    gs::Image shadow;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dartseven
