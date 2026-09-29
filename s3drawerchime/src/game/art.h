// S3 DRAWERCHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace drawerchime {

constexpr int kTarget = 4;
constexpr int kTries = 3;
constexpr int kGraceSec = 16;
constexpr int kFpc = 4;
constexpr int kHourSec = kTarget * 3600;
constexpr int kStartSec = kHourSec - 36;
constexpr int kDrawers = 4;  // hour, minute, second, chime

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_WOOD = 4,
    PAL_BRASS = 5,
    PAL_BELL = 6,
    PAL_FACE = 7,
    PAL_LIT = 8,
    PAL_CLERK = 9,
    PAL_NIGHT = 10
};

struct Art {
    gs::Image cabinet;
    gs::Image drawer;
    gs::Image chimeBox;
    gs::Image knob;
    gs::Image bell;
    gs::Image clapper;
    gs::Image face;
    gs::Image handH[12];
    gs::Image handM[12];
    gs::Image clerk;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawerchime
