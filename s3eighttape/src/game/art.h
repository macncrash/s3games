// S3 EIGHTTAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace eighttape {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_WIN = 3,
    PAL_BALL = 4,
    PAL_TABLE = 5,
    PAL_CUE = 6,
    PAL_PAPER = 7,
    PAL_SOLID = 8,
    PAL_STRIPE = 9,
    PAL_PLAYER = 10,
    PAL_AIM = 11,
    PAL_GLOW = 12,
    PAL_LAMP = 13,
    PAL_SHADE = 14
};

constexpr float kR = 6.5f;
constexpr float kFeltL = 20.f;
constexpr float kFeltT = 44.f;
constexpr float kFeltR = 196.f;
constexpr float kFeltB = 176.f;
constexpr float kRailL = 8.f;
constexpr float kRailT = 28.f;
constexpr float kRailR = 200.f;
constexpr float kRailB = 184.f;
constexpr float kPaperL = 206.f;
constexpr float kPaperT = 26.f;
constexpr float kPaperR = 314.f;
constexpr float kPaperB = 156.f;
constexpr float kSlotX0 = 224.f;
constexpr float kSlotY = 174.f;
constexpr float kSlotPitch = 30.f;
constexpr int kBalls = 5;
constexpr int kTapeN = 3;
constexpr int kSpotBall = 4;
constexpr int kSpotPocket = 2;
constexpr int kSpotPay = 8;
constexpr int kMaxStrokes = 5;
constexpr int kCueAngles = 16;
constexpr int kCueBox = 40;
constexpr float kTip = 16.f;

struct Pocket {
    float x, y, sink;
    const char* name;
};

// 0 corner (the eight), 1 side (the solid), 2 wide (the spot), 4 foot (the stripe).
constexpr Pocket kPocket[6] = {
    {20.f, 44.f, 15.f, "CORNER"}, {108.f, 38.f, 16.f, "SIDE"}, {196.f, 44.f, 15.f, "WIDE"},
    {20.f, 176.f, 13.f, "LEFT"},  {108.f, 182.f, 16.f, "FOOT"}, {196.f, 176.f, 13.f, "RIGHT"},
};

inline Pocket pocketAt(int i) {
    if (i < 0 || i > 5) i = 0;
    return kPocket[i];
}

struct Line {
    const char* name;
    int pay;
    int ball;
    int pocket;
};

// The tape. The spot pays the same 8 and is not a line.
constexpr Line kTape[kTapeN] = {
    {"EIGHT", 8, 1, 0},
    {"SOLID", 3, 2, 1},
    {"STRIPE", 5, 3, 4},
};

inline Line tapeAt(int i) {
    if (i < 0 || i >= kTapeN) i = 0;
    return kTape[i];
}

inline int homePocket(int ball) {
    if (ball == 1) return 0;
    if (ball == 2) return 1;
    if (ball == 3) return 4;
    if (ball == kSpotBall) return kSpotPocket;
    return -1;
}

struct Art {
    int font[96] = {};
    gs::Mipped ball[kBalls];
    gs::Mipped shadow;
    gs::Mipped cue[kCueAngles];
    gs::Mipped dot;
    gs::Mipped ring;
    gs::Mipped slip;
    gs::Mipped slot;
    gs::Mipped lamp;
    gs::Mipped player[2];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eighttape
