// S3 DART GOLD pictures. Drawn at boot. No asset files.
// The paint and the score are the same test: gold is a double, cream is not.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace dartgold {

constexpr int kBoard = 180;
constexpr float kBmpC = 89.5f;
constexpr int kBoardX = 70;
constexpr int kBoardY = 26;
constexpr float kCx = kBoardX + kBmpC;
constexpr float kCy = kBoardY + kBmpC;

constexpr float kBullIn = 7.f;
constexpr float kBullOut = 15.f;
constexpr float kTripIn = 34.f;
constexpr float kTripOut = 46.f;
constexpr float kDoubIn = 58.f;
constexpr float kDoubOut = 70.f;
constexpr float kWoodIn = 74.f;
constexpr float kNumR = 80.f;
constexpr float kRim = 87.f;

constexpr float kR25 = (kBullIn + kBullOut) * 0.5f;
constexpr float kRTrip = (kTripIn + kTripOut) * 0.5f;
constexpr float kRSing = (kTripOut + kDoubIn) * 0.5f;
constexpr float kRDoub = (kDoubIn + kDoubOut) * 0.5f;

// Clockwise from the top, match order. Even indexes are the gold beds.
constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};

constexpr int kPaintBlack = 1;
constexpr int kPaintCream = 3;
constexpr int kPaintGold = 5;
constexpr int kPaintPale = 6;
constexpr int kPaintRed = 7;
constexpr int kPaintGreen = 8;
constexpr int kPaintWire = 9;

enum Pal {
    PAL_BOARD = 0,
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_CREAM = 3,
    PAL_GREEN = 4,
    PAL_RED = 5,
    PAL_DIM = 6,
    PAL_SCORE = 7,
    PAL_TITLE = 8,
    PAL_BUST = 9,
    PAL_WIN = 10,
    PAL_AIM = 11,
    PAL_SWEET = 12,
    PAL_DART0 = 13,
    PAL_DART1 = 14,
    PAL_DART2 = 15
};

inline bool goldSector(int s) { return (s & 1) == 0; }

// One scoring cell. kind: M miss, B bull, O outer bull, T S D C.
struct Zone {
    int score = 0;
    int face = 0;
    bool dbl = false;
    bool gold = false;
    int paint = 0;
    char kind = 'M';
};

Zone zoneAt(float dx, float dy);

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

}  // namespace dartgold
