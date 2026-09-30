// S3 BIKE BOOM pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikeboom {

constexpr float kRack = 0.92f;
constexpr float kRear = 0.88f;
constexpr float kFront = 1.28f;
constexpr float kBoom = 72.0f;
constexpr float kWindow = 0.82f;
constexpr float kPpm = 12.6f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_DRIVE = 2,
    PAL_ROAD = 3,
    PAL_BOOM = 4,
    PAL_HOOK = 5,
    PAL_CREW = 6,
    PAL_CLOCK = 7,
    PAL_TOWN = 8,
    PAL_DUST = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_BANNER = 12,
    PAL_DOCK = 13,
};

struct Art {
    gs::Mipped bike;
    gs::Mipped wheel[2];
    gs::Mipped drive;
    gs::Mipped reel;
    gs::Mipped dust;
    gs::Mipped road;
    gs::Mipped kerb;
    gs::Mipped mast;
    gs::Mipped arm;
    gs::Mipped hook;
    gs::Mipped pad;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped van;
    gs::Mipped lamp;
    gs::Mipped tree;
    gs::Mipped title;
    gs::Mipped boomWord;
    gs::Mipped delivered;
    gs::Mipped shortB;
    gs::Mipped past;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikeboom
