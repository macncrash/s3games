// S3 EIGHTMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace eightmark {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_BALL = 3,
    PAL_TABLE = 4,
    PAL_CUE = 5,
    PAL_GLOW = 6,
    PAL_LOGO = 7,
    PAL_COIN = 8,
    PAL_HAND = 9
};

constexpr float kTableX = 16.f;
constexpr float kTableY = 20.f;
constexpr float kTableW = 288.f;
constexpr float kTableH = 180.f;
constexpr float kFeltL = 32.f;
constexpr float kFeltT = 36.f;
constexpr float kFeltR = 288.f;
constexpr float kFeltB = 184.f;
constexpr float kR = 7.f;
constexpr float kCx = 160.f;
constexpr float kCy = 110.f;
constexpr float kCueX = 160.f;
constexpr float kCueY = 162.f;
constexpr float kEightX = 160.f;
constexpr float kEightY = 110.f;
constexpr float kCoinX = 72.f;
constexpr float kCoinY = 128.f;
constexpr float kHandX = 206.f;
constexpr float kHandY = 168.f;
constexpr int kBalls = 6;
constexpr int kCueAngles = 32;
constexpr int kCueBox = 40;

struct Pocket {
    float x, y, sink;
    const char* name;
};

inline Pocket pocketAt(int i) {
    static const Pocket kP[6] = {
        {32.f, 36.f, 20.f, "TOP LEFT"},  {160.f, 30.f, 22.f, "TOP SIDE"},  {288.f, 36.f, 20.f, "TOP RIGHT"},
        {32.f, 184.f, 20.f, "BOT LEFT"}, {160.f, 190.f, 22.f, "BOT SIDE"}, {288.f, 184.f, 20.f, "BOT RIGHT"},
    };
    if (i < 0 || i > 5) i = 0;
    return kP[i];
}

// kind: 0 cue, 1 solid, 2 stripe, 3 eight. color is a PAL_BALL index.
struct Home {
    float x, y;
    int kind, color;
};

inline Home homeAt(int i) {
    static const Home kH[kBalls] = {
        {kCueX, kCueY, 0, 1},     {kEightX, kEightY, 3, 2}, {240.f, 100.f, 1, 3},
        {226.f, 116.f, 1, 5},     {254.f, 116.f, 2, 4},     {240.f, 132.f, 2, 7},
    };
    if (i < 0 || i >= kBalls) i = 0;
    return kH[i];
}

struct Art {
    int font[96] = {};
    gs::Mipped ball[kBalls];
    gs::Mipped shadow;
    gs::Mipped cue[kCueAngles];
    gs::Mipped dot;
    gs::Mipped glow;
    gs::Mipped coin;
    gs::Mipped glove;
    gs::Mipped logo;
    gs::Mipped sub;
    gs::Mipped win;
    gs::Mipped lose;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eightmark
