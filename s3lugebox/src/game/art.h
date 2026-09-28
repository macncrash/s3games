// S3 LUGE BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lugebox {

constexpr float kHalf = 3.4f;
constexpr float kBoxL = 108.f;
constexpr float kBoxR = 128.f;
constexpr float kMargin = 0.45f;
constexpr float kGoal = 118.f;
constexpr float kPpm = 6.4f;
constexpr float kIceY = 150.f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_LUGE = 1,
    PAL_ICE = 2,
    PAL_POST = 3,
    PAL_BOX = 4,
    PAL_CREW = 5,
    PAL_CLOCK = 6,
    PAL_SPRAY = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_HUT = 11,
    PAL_PINE = 12,
    PAL_BIRD = 13,
    PAL_SNOW = 14
};

struct Art {
    gs::Mipped luge;
    gs::Mipped spray;
    gs::Mipped flake;
    gs::Mipped bank;
    gs::Mipped pine;
    gs::Mipped post;
    gs::Mipped stripe;
    gs::Mipped hut;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped lamp;
    gs::Mipped bird[2];
    gs::Mipped title;
    gs::Mipped boxWord;
    gs::Mipped stopped;
    gs::Mipped outside;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lugebox
