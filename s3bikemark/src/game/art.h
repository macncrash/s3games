// S3 BIKE MARK pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikemark {

constexpr float kRear = 0.78f;
constexpr float kFront = 1.08f;
constexpr float kMarkL = 58.2f;
constexpr float kMarkR = 61.55f;
constexpr float kGoal = 59.85f;
constexpr float kPpm = 14.0f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_RIDER = 2,
    PAL_ROAD = 3,
    PAL_MARK = 4,
    PAL_STAND = 5,
    PAL_CREW = 6,
    PAL_CLOCK = 7,
    PAL_YARD = 8,
    PAL_DUST = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_BANNER = 12,
    PAL_SKY = 13,
};

struct Art {
    gs::Mipped frame;
    gs::Mipped wheel[2];
    gs::Mipped stand;
    gs::Mipped dust;
    gs::Mipped road;
    gs::Mipped paint;
    gs::Mipped cross;
    gs::Mipped peg;
    gs::Mipped yard;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped lamp;
    gs::Mipped hedge;
    gs::Mipped title;
    gs::Mipped markWord;
    gs::Mipped setWord;
    gs::Mipped offWord;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikemark
