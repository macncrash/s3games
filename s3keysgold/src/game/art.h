// S3 KEYS GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keysgold {

constexpr int kLanes = 4;
constexpr int kLaneX[kLanes] = {72, 120, 168, 216};
constexpr int kHitY = 156;
constexpr float kTravel = 118.f;
constexpr int kLamps = 4;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_KEY = 3,
    PAL_KEYLIT = 4,
    PAL_LINE = 5,
    PAL_BAD = 6,
    PAL_WHITE = 7,
    PAL_WOOD = 8
};

struct Art {
    gs::Mipped gold;
    gs::Mipped cream;
    gs::Mipped key[kLanes];
    gs::Mipped rail;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keysgold
