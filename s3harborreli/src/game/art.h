// S3 HARBOR RELIEF pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace harbor {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_KEEPER = 4,
    PAL_CUTTER = 5,
    PAL_BARGE = 6,
    PAL_LAUNCH = 7,
    PAL_STONE = 8,
    PAL_BELL = 9,
    PAL_WOOD = 10,
    PAL_NIGHT = 11,
    PAL_WATER = 12,
    PAL_LIGHT = 13,
    PAL_FX = 14
};

inline constexpr float kBoomZ = 0.78f;
inline constexpr float kEnterZ = 1.04f;
inline constexpr float kHorizon = 86.f;

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped cutter[2];
    gs::Mipped barge;
    gs::Mipped launch[2];
    gs::Mipped chain, bell, rope, light, buoy, gull, splash, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int stone = 1;
    int cap = 2;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harbor
