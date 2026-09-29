// S3 BIKE GRASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikegrass {

constexpr float kRear = 0.62f;
constexpr float kFront = 0.78f;
constexpr float kRamp0 = 12.f;
constexpr float kLip = 22.f;
constexpr float kGrass0 = 29.2f;
constexpr float kGrass1 = 47.5f;
constexpr float kLipH = 3.35f;
constexpr float kGrassH = 0.28f;
constexpr float kPpm = 12.2f;
constexpr float kBaseY = 176.f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_DIRT = 2,
    PAL_GRASS = 3,
    PAL_TREE = 4,
    PAL_FLAG = 5,
    PAL_DUST = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_PIT = 10,
    PAL_RAMP = 11,
};

struct Art {
    gs::Mipped bike;
    gs::Mipped wheel[2];
    gs::Mipped dirt;
    gs::Mipped grass;
    gs::Mipped tuft;
    gs::Mipped ramp;
    gs::Mipped pit;
    gs::Mipped tree;
    gs::Mipped flag;
    gs::Mipped dust;
    gs::Mipped title;
    gs::Mipped grassWord;
    gs::Mipped stopped;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikegrass
