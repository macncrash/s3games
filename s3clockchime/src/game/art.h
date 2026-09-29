// S3 CLOCKCHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/vdp.h"

namespace clockchime {

constexpr int kFace = 112;
constexpr int kPivot = 40;
constexpr int kHandN = 60;
constexpr int kCx = 118;
constexpr int kCy = 112;
constexpr int kBellX = 250;
constexpr int kBellY = 52;

constexpr int kTarget = 4;
constexpr int kRopes = 3;
constexpr int kGraceSec = 24;
constexpr int kFpc = 6;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 48;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_FACE = 4,
    PAL_HOUR = 5,
    PAL_MIN = 6,
    PAL_SEC = 7,
    PAL_BELL = 8,
    PAL_KEEP = 9,
    PAL_NIGHT = 10,
    PAL_LIT = 11
};

struct Art {
    gs::Image face;
    gs::Image hand[3][kHandN];
    gs::Image bell, rope, cap, keeper, moon, star;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace clockchime
