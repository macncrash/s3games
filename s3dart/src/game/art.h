// S3 DART pictures. Drawn at boot. No asset files.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace dart {

constexpr int kBoard = 196;
constexpr float kBmpC = 97.5f;
constexpr int kBoardX = 62;
constexpr int kBoardY = 26;
constexpr float kCx = kBoardX + kBmpC;
constexpr float kCy = kBoardY + kBmpC;

// Scoring radii, bitmap pixels. The picture and the rules share these.
constexpr float kBullIn = 6.5f;
constexpr float kBullOut = 14.f;
constexpr float kTripIn = 41.f;
constexpr float kTripOut = 53.f;
constexpr float kDoubIn = 67.f;
constexpr float kDoubOut = 79.f;
constexpr float kWoodIn = 90.f;
constexpr float kNumR = 84.5f;
constexpr float kRim = 96.f;

// Clockwise from the top, as on a match board. 20 is black and red.
constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};

enum Pal {
    PAL_BOARD = 0,
    PAL_SCORE = 1,
    PAL_INK = 2,
    PAL_GOLD = 3,
    PAL_RED = 4,
    PAL_GREEN = 5,
    PAL_AIM = 6,
    PAL_SWEET = 7,
    PAL_DART0 = 8,
    PAL_DART1 = 9,
    PAL_DART2 = 10,
    PAL_TITLE = 11,
    PAL_BUST = 12,
    PAL_WIN = 13
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
    gs::Image digit[10];
    int digitW = 0;
    int digitH = 0;
    gs::Image title;
    gs::Image bust;
    gs::Image win;
    gs::Mipped dart;
    gs::Image cross;
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dart
