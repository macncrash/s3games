// S3 EIGHT GOLD. Sprites and the table are drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace eightgold {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_RED = 3,
    PAL_BALL = 4,
    PAL_TABLE = 5,
    PAL_CUE = 6,
    PAL_LOGO = 7
};

// Screen table. Felt is left transparent so the backdrop reads as cloth.
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
constexpr int kRack = 8;

struct Pocket {
    float x, y, sink;
    bool gold;
    const char* name;
};

// Corners count double. Side pockets are cream and will not clear the 8.
inline Pocket pocketAt(int i) {
    static const Pocket kP[6] = {
        {24.f, 32.f, 22.f, true, "TOP LEFT"},   {160.f, 28.f, 20.f, false, "TOP SIDE"},
        {296.f, 32.f, 22.f, true, "TOP RIGHT"}, {24.f, 192.f, 22.f, true, "BOT LEFT"},
        {160.f, 196.f, 20.f, false, "BOT SIDE"}, {296.f, 192.f, 22.f, true, "BOT RIGHT"},
    };
    if (i < 0 || i > 5) i = 0;
    return kP[i];
}

inline bool pocketGold(int i) { return pocketAt(i).gold; }

struct Art {
    gs::Mipped ball[kRack + 1];
    gs::Mipped shadow;
    gs::Mipped cue[kCueAngles];
    gs::Mipped dot;
    gs::Mipped glow;
    gs::Mipped logo;
    gs::Mipped sub;
    gs::Mipped win;
    gs::Mipped tag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace eightgold
