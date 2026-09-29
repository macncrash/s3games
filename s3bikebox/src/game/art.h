// S3 BIKE BOX pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikebox {

constexpr float kRear = 0.95f;
constexpr float kFront = 1.20f;
constexpr float kBoxL = 68.0f;
constexpr float kBoxR = 74.6f;
constexpr float kGoal = 71.25f;
constexpr float kPpm = 13.5f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_RIDER = 2,
    PAL_ROAD = 3,
    PAL_BOX = 4,
    PAL_POST = 5,
    PAL_CREW = 6,
    PAL_CLOCK = 7,
    PAL_TOWN = 8,
    PAL_DUST = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_BANNER = 12,
    PAL_SKY = 13,
};

struct Art {
    gs::Mipped bike;
    gs::Mipped wheel[2];
    gs::Mipped dust;
    gs::Mipped road;
    gs::Mipped stripe;
    gs::Mipped kerb;
    gs::Mipped post;
    gs::Mipped hatch;
    gs::Mipped shed;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped lamp;
    gs::Mipped tree;
    gs::Mipped title;
    gs::Mipped boxWord;
    gs::Mipped stopped;
    gs::Mipped outside;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikebox
