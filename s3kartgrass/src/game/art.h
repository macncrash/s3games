// S3 KART GRASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartgrass {

constexpr float kRear = 0.90f;
constexpr float kFront = 1.12f;
constexpr float kRamp0 = 11.5f;
constexpr float kLip = 21.2f;
constexpr float kGrass0 = 28.4f;
constexpr float kGrass1 = 51.0f;
constexpr float kLipH = 3.20f;
constexpr float kGrassH = 0.32f;
constexpr float kPpm = 11.6f;
constexpr float kBaseY = 178.f;
constexpr float kCrew = 15.5f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_KART = 1,
    PAL_DIRT = 2,
    PAL_GRASS = 3,
    PAL_TREE = 4,
    PAL_FLAG = 5,
    PAL_DUST = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_GAP = 10,
    PAL_RAMP = 11,
    PAL_CREW = 12,
};

struct Art {
    gs::Mipped kart;
    gs::Mipped tyre[2];
    gs::Mipped dirt;
    gs::Mipped grass;
    gs::Mipped tuft;
    gs::Mipped ramp;
    gs::Mipped gap;
    gs::Mipped tree;
    gs::Mipped flag;
    gs::Mipped dust;
    gs::Mipped clock;
    gs::Mipped title;
    gs::Mipped grassWord;
    gs::Mipped stopped;
    gs::Mipped missed;
    gs::Mipped crew;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartgrass
