// S3 EIGHT sprites and the table. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace eight {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_BALL = 3,
    PAL_TABLE = 4,
    PAL_CUE = 5,
    PAL_GLOW = 6,
    PAL_LOGO = 7
};

// Screen table. Felt is a hole in the plane so the backdrop reads as cloth.
constexpr float kTableX = 8.f;
constexpr float kTableY = 16.f;
constexpr float kTableW = 304.f;
constexpr float kTableH = 192.f;
constexpr float kFeltL = 24.f;
constexpr float kFeltT = 32.f;
constexpr float kFeltR = 296.f;
constexpr float kFeltB = 192.f;
constexpr float kBallR = 8.f;
constexpr float kMidY = 112.f;
constexpr float kHeadX = 74.f;
constexpr float kFootX = 206.f;
constexpr int kCueAngles = 32;
constexpr int kCueBox = 48;
constexpr float kCueTip = 17.f;

struct Pocket {
    float x, y, sink;
    const char* name;
};

inline Pocket pocketAt(int i) {
    static const Pocket kP[6] = {
        {24.f, 32.f, 22.f, "TOP LEFT"},  {160.f, 28.f, 20.f, "TOP SIDE"},  {296.f, 32.f, 22.f, "TOP RIGHT"},
        {24.f, 192.f, 22.f, "BOT LEFT"}, {160.f, 196.f, 20.f, "BOT SIDE"}, {296.f, 192.f, 22.f, "BOT RIGHT"},
    };
    if (i < 0 || i > 5) i = 0;
    return kP[i];
}

struct Art {
    gs::Mipped ball[16];
    gs::Mipped shadow;
    gs::Mipped cue[kCueAngles];
    gs::Mipped dot;
    gs::Mipped glow;
    gs::Mipped logo;
    gs::Mipped sub;
    gs::Mipped win;
    gs::Mipped lose;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eight
