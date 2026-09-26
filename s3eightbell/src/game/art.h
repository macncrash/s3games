// S3 EIGHTBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace eightbell {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_BALL = 3,
    PAL_TABLE = 4,
    PAL_CUE = 5,
    PAL_GLOW = 6,
    PAL_LOGO = 7,
    PAL_BELL = 8,
    PAL_WIN = 9,
    PAL_FLAME = 10,
    PAL_DIM = 11
};

constexpr float kTableX = 16.f;
constexpr float kTableY = 32.f;
constexpr float kTableW = 288.f;
constexpr float kTableH = 176.f;
constexpr float kFeltL = 34.f;
constexpr float kFeltT = 50.f;
constexpr float kFeltR = 286.f;
constexpr float kFeltB = 190.f;
constexpr float kR = 7.f;
constexpr int kBell = 1;
constexpr int kBalls = 6;
constexpr int kCueAngles = 24;
constexpr int kCueBox = 56;
constexpr float kTip = 22.f;

struct Pocket {
    float x, y, sink;
    const char* name;
};

inline Pocket pocketAt(int i) {
    static const Pocket kP[6] = {
        {34.f, 50.f, 17.f, "CORNER"}, {160.f, 44.f, 21.f, "BELL"},  {286.f, 50.f, 17.f, "CORNER"},
        {34.f, 190.f, 17.f, "CORNER"}, {160.f, 198.f, 18.f, "FOOT"}, {286.f, 190.f, 17.f, "CORNER"},
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
        {160.f, 166.f, 0, 1}, {164.f, 106.f, 3, 2}, {230.f, 100.f, 1, 3},
        {246.f, 114.f, 1, 5}, {216.f, 124.f, 2, 4}, {234.f, 138.f, 2, 7},
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
    gs::Mipped clapper;
    gs::Mipped yoke;
    gs::Mipped lamp;
    gs::Mipped flame;
    gs::Mipped eight;
    gs::Mipped bellWord;
    gs::Mipped left;
    gs::Mipped dead;
    gs::Mipped rung;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eightbell
