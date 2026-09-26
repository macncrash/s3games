// Pictures drawn at boot. The board radii are also the scoring rules.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace dartmark {

constexpr int kBoard = 152;
constexpr float kBmpC = 76.f;
constexpr int kBoardX = 84;
constexpr int kBoardY = 44;
constexpr float kCx = kBoardX + kBmpC;
constexpr float kCy = kBoardY + kBmpC;

// Bitmap pixels, shared by the painted board and the scorer.
constexpr float kBullIn = 5.f;
constexpr float kBullOut = 11.f;
constexpr float kTripIn = 32.f;
constexpr float kTripOut = 42.f;
constexpr float kDoubIn = 52.f;
constexpr float kDoubOut = 62.f;
constexpr float kWoodIn = 66.f;
constexpr float kNumR = 69.f;
constexpr float kRim = 74.f;

// Clockwise from the top. 20 is the mark.
constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};
constexpr int kMarkSeg = 0;
constexpr int kMarkNum = 20;

constexpr float kSlateX = 6.f;
constexpr float kSlateY = 72.f;

enum Pal {
    PAL_BOARD = 0,
    PAL_SLATE = 1,
    PAL_INK = 2,
    PAL_GOLD = 3,
    PAL_GREEN = 4,
    PAL_RED = 5,
    PAL_AIM = 6,
    PAL_DART0 = 7,
    PAL_DART1 = 8,
    PAL_DART2 = 9,
    PAL_TITLE = 10,
    PAL_WIN = 11,
    PAL_CHALK = 12,
    PAL_PIP = 13,
    PAL_LOSE = 14
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
    gs::Image slate;
    gs::Image title;
    gs::Image win;
    gs::Image lose;
    gs::Mipped dart;
    gs::Image cross;
    gs::Image dot;
    gs::Image slash;
    gs::Image pip;
    gs::Image shadow;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dartmark
