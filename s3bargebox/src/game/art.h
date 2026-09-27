// S3 BARGE BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargebox {

constexpr float kHalf = 9.f;
constexpr float kBoxL = 96.f;
constexpr float kBoxR = 124.f;
constexpr float kMargin = 0.40f;
constexpr float kGoal = 110.f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_BARGE = 1,
    PAL_BANK = 2,
    PAL_POST = 3,
    PAL_BOX = 4,
    PAL_CREW = 5,
    PAL_CLOCK = 6,
    PAL_FOAM = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_SHED = 11,
    PAL_TREE = 12,
    PAL_BIRD = 13,
};

struct Art {
    gs::Mipped barge;
    gs::Mipped wake;
    gs::Mipped smoke;
    gs::Mipped bank;
    gs::Mipped reed;
    gs::Mipped post;
    gs::Mipped pad;
    gs::Mipped hatch;
    gs::Mipped shed;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped tree;
    gs::Mipped bird[2];
    gs::Mipped lamp;
    gs::Mipped title;
    gs::Mipped boxWord;
    gs::Mipped stopped;
    gs::Mipped outside;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargebox
