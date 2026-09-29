// S3 FISHCHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/vdp.h"

namespace fishchime {

constexpr int kFace = 40;
constexpr int kPivot = 16;
constexpr int kHandN = 12;

constexpr int kFpc = 6;
constexpr int kGraceSec = 16;
constexpr int kTarget = 12;
constexpr int kBreaths = 3;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 40;
constexpr float kBellX = 268.f;
constexpr float kFishY = 150.f;
constexpr float kReach = 16.f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_FISH = 2,
    PAL_BELL = 3,
    PAL_KELP = 4,
    PAL_BUOY = 5,
    PAL_BAD = 6,
    PAL_FACE = 7,
    PAL_DIM = 8,
    PAL_BUB = 9
};

struct Art {
    gs::Image fish[2];
    gs::Image bell, buoy, kelp, pier, bubble, face;
    gs::Image hand[2][kHandN];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fishchime
