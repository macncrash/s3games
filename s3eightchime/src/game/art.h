// S3 EIGHTCHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace eightchime {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_BALL = 3,
    PAL_TABLE = 4,
    PAL_CUE = 5,
    PAL_GLOW = 6,
    PAL_FACE = 7,
    PAL_HAND = 8,
    PAL_BELL = 9,
    PAL_WIN = 10,
    PAL_DIM = 11
};

constexpr float kFeltL = 32.f;
constexpr float kFeltT = 72.f;
constexpr float kFeltR = 288.f;
constexpr float kFeltB = 210.f;
constexpr float kR = 7.f;
constexpr int kHourPocket = 2;
constexpr int kBalls = 4;
constexpr int kCueAngles = 24;
constexpr int kCueBox = 64;
constexpr int kDial = 40;

struct Pocket {
    float x, y, sink;
    const char* name;
};

inline Pocket pocketAt(int i) {
    static const Pocket kP[6] = {
        {32.f, 72.f, 16.f, "CORNER"}, {160.f, 70.f, 14.f, "SIDE"}, {288.f, 72.f, 18.f, "HOUR"},
        {32.f, 210.f, 16.f, "CORNER"}, {160.f, 214.f, 14.f, "FOOT"}, {288.f, 210.f, 16.f, "CORNER"},
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
        {154.f, 168.f, 0, 1}, {198.f, 136.f, 3, 2}, {58.f, 112.f, 1, 3}, {86.f, 186.f, 2, 7},
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
    gs::Mipped ring;
    gs::Mipped bell;
    gs::Mipped plaque;
    gs::Image hand[3][60];
    gs::Image face;
    gs::Image bezel;
    gs::Image cap;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eightchime
