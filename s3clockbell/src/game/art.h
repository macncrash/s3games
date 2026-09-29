// S3 CLOCKBELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace clockbell {

constexpr int kCx = 156;
constexpr int kCy = 118;
constexpr int kPivot = 40;
constexpr int kSteps = 60;
constexpr int kTarget = 5;  // the hour the bell will answer
constexpr int kMaxDead = 3;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_DIAL = 4,
    PAL_HOUR = 5,
    PAL_MIN = 6,
    PAL_BELL = 7,
    PAL_LIT = 8,
    PAL_TOWER = 9,
    PAL_ROPE = 10,
    PAL_SKY = 11
};

struct Art {
    gs::Image hour[kSteps];
    gs::Image minute[kSteps];
    gs::Image dial;
    gs::Image bell;
    gs::Image rope;
    gs::Image cap;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clockbell
