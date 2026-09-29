// S3 CLOCKTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace clocktape {

constexpr int kCx = 112;
constexpr int kCy = 108;
constexpr int kPivot = 36;
constexpr int kSteps = 60;
constexpr int kTapeN = 3;
constexpr int kFaults = 4;

struct Slip {
    int hour;     // 1..12
    int minute;   // 0, 15, 30, 45
    const char* label;
};

// The tape. A face that is merely close stays out of the drawer.
constexpr Slip kTape[kTapeN] = {
    {4, 0, "4:00"},
    {7, 30, "7:30"},
    {10, 15, "10:15"},
};

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_DIAL = 4,
    PAL_HOUR = 5,
    PAL_MIN = 6,
    PAL_WOOD = 7,
    PAL_TAPE = 8,
    PAL_INK = 9,
    PAL_SLIP = 10,
    PAL_CAB = 11,
    PAL_LIT = 12
};

struct Art {
    gs::Image hour[kSteps];
    gs::Image minute[kSteps];
    gs::Image dial;
    gs::Image cap;
    gs::Image cabinet;
    gs::Image tape;
    gs::Image drawer;
    gs::Image slip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clocktape
